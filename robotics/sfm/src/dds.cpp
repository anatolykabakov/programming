#include "dds.h"
#include <atom/dds/msgs.hpp>
#include <atom/dds/car.hpp>
#include <atom/dds/sensors.hpp>
#include <dds/dds.hpp>
#include "pcl/common/transforms.h"
#include <opencv2/imgcodecs.hpp>

CycloneDDS::CycloneDDS(Eigen::Isometry3d extrinsic) : extrinsic_(extrinsic)
{
  dds::domain::DomainParticipant participant{0};
  dds::pub::qos::DataWriterQos wqos;
  wqos << dds::core::policy::Reliability::BestEffort();

  dds::topic::Topic<sensor_msgs::msg::dds_::PointCloud2_> cloud_topic{participant, "rt/cloud"};
  cloud_writer_ =
      std::make_shared<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>>(participant, cloud_topic, wqos);

  dds::topic::Topic<sensor_msgs::msg::dds_::PointCloud2_> poses_topic{participant, "rt/poses"};
  poses_writer_ =
      std::make_shared<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>>(participant, poses_topic, wqos);

  dds::topic::Topic<geometry_msgs::msg::dds_::PoseStamped_> pose_topic{participant, "rt/pose"};
  pose_writer_ =
      std::make_shared<dds::pub::DataWriter<geometry_msgs::msg::dds_::PoseStamped_>>(participant, pose_topic, wqos);

  dds::topic::Topic<geometry_msgs::msg::dds_::PoseStamped_> gt_topic{participant, "rt/gt"};
  gt_writer_ =
      std::make_shared<dds::pub::DataWriter<geometry_msgs::msg::dds_::PoseStamped_>>(participant, gt_topic, wqos);

  dds::topic::Topic<sensor_msgs::msg::dds_::CompressedImage_> img_topic{participant, "rt/img"};
  img_writer_ =
      std::make_shared<dds::pub::DataWriter<sensor_msgs::msg::dds_::CompressedImage_>>(participant, img_topic, wqos);
}

void CycloneDDS::publishCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                              std::shared_ptr<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>> writer)
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto time =
      builtin_interfaces::msg::dds_::Time_(std::chrono::duration_cast<std::chrono::seconds>(now).count(), 0);

  pcl::PCLPointCloud2 pcl_msg;
  pcl::toPCLPointCloud2(*cloud, pcl_msg);
  std::vector<sensor_msgs::msg::dds_::PointField_> fields(pcl_msg.fields.size());
  for (size_t index = 0; index < fields.size(); index++) {
    const auto& field = pcl_msg.fields[index];
    fields[index] = sensor_msgs::msg::dds_::PointField_(field.name, field.offset, field.datatype, field.count);
  }
  auto pcl_to_send = sensor_msgs::msg::dds_::PointCloud2_(
      std_msgs::msg::dds_::Header_(time, "base_link"), pcl_msg.height, pcl_msg.width, fields, pcl_msg.is_bigendian,
      pcl_msg.point_step, pcl_msg.row_step, pcl_msg.data, pcl_msg.is_dense);

  writer->write(std::move(pcl_to_send));
}

void CycloneDDS::publish(const cv::Mat& image)
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto time =
      builtin_interfaces::msg::dds_::Time_(std::chrono::duration_cast<std::chrono::seconds>(now).count(), 0);

  if (!image.empty()) {
    std::vector<uint8_t> buf;
    cv::imencode(".png", image, buf);
    sensor_msgs::msg::dds_::CompressedImage_ ros_image(std_msgs::msg::dds_::Header_(time, "base_link"), "png", buf);
    img_writer_->write(std::move(ros_image));
  }
}

void CycloneDDS::publish(const Eigen::Isometry3d& pose, const std::string& type)
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto time =
      builtin_interfaces::msg::dds_::Time_(std::chrono::duration_cast<std::chrono::seconds>(now).count(),
                                           std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());

  auto pose_base = extrinsic_.inverse() * pose;
  const auto quat = Eigen::Quaterniond(pose_base.linear().matrix());

  const auto header = std_msgs::msg::dds_::Header_(time, "base_link");
  const auto p = geometry_msgs::msg::dds_::Point_(pose_base.translation().x(), pose_base.translation().y(),
                                                  pose_base.translation().z());
  const auto q = geometry_msgs::msg::dds_::Quaternion_(quat.x(), quat.y(), quat.z(), quat.w());
  const auto pose_msg = geometry_msgs::msg::dds_::Pose_(p, q);
  const auto pose_stamped_msg = geometry_msgs::msg::dds_::PoseStamped_(header, pose_msg);

  if (type == "gt") {
    gt_writer_->write(std::move(pose_stamped_msg));
  } else {
    pose_writer_->write(std::move(pose_stamped_msg));
  }
}

void CycloneDDS::publish(const openMVG::sfm::SfM_Data& scene)
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

  for (const auto [id, landmark] : scene.structure) {
    pcl::PointXYZ p;
    p.x = landmark.X.x();
    p.y = landmark.X.y();
    p.z = landmark.X.z();
    cloud->push_back(p);
  }

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_base(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::transformPointCloud(*cloud, *cloud_base, extrinsic_.inverse().matrix());

  publishCloud(cloud_base, cloud_writer_);

  pcl::PointCloud<pcl::PointXYZ>::Ptr poses_cloud(new pcl::PointCloud<pcl::PointXYZ>);
  for (const auto& [id, pose] : scene.poses) {
    pcl::PointXYZ p;
    const auto& position = pose.translation();
    p.x = position.x();
    p.y = position.y();
    p.z = position.z();
    poses_cloud->push_back(p);
  }

  pcl::PointCloud<pcl::PointXYZ>::Ptr poses_cloud_base(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::transformPointCloud(*poses_cloud, *poses_cloud_base, extrinsic_.inverse().matrix());

  publishCloud(poses_cloud_base, poses_writer_);
}

void CycloneDDS::publish(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud)
{
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_base(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::transformPointCloud(*cloud, *cloud_base, extrinsic_.inverse().matrix());

  publishCloud(cloud_base, cloud_writer_);
}
