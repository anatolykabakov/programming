#pragma once
#include <memory>
#include <dds/pub/DataWriter.hpp>
#include <dds/pub/qos/DataWriterQos.hpp>
#include <atom/dds/ros2.hpp>

#include "pcl/PCLPointCloud2.h"
#include "pcl/impl/point_types.hpp"
#include "pcl/point_cloud.h"
#include "pcl/conversions.h"

#include "opencv2/core/mat.hpp"

#include "openMVG/sfm/sfm_data.hpp"

class CycloneDDS {
public:
  CycloneDDS(Eigen::Isometry3d extrinsic);

  void publish(const openMVG::sfm::SfM_Data& scene);

  void publish(const Eigen::Isometry3d& pose, const std::string& type);

  void publish(const cv::Mat& image);

  void publish(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud);

private:
  Eigen::Isometry3d extrinsic_;
  std::shared_ptr<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>> cloud_writer_;
  std::shared_ptr<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>> poses_writer_;
  std::shared_ptr<dds::pub::DataWriter<geometry_msgs::msg::dds_::PoseStamped_>> pose_writer_;
  std::shared_ptr<dds::pub::DataWriter<geometry_msgs::msg::dds_::PoseStamped_>> gt_writer_;
  std::shared_ptr<dds::pub::DataWriter<sensor_msgs::msg::dds_::CompressedImage_>> img_writer_;

  void publishCloud(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                    std::shared_ptr<dds::pub::DataWriter<sensor_msgs::msg::dds_::PointCloud2_>> writer);
};
