// https://gist.github.com/cashiwamochi/8ac3f8bab9bf00e247a01f63075fedeb

#include <opencv2/opencv.hpp>

#include <pcl/common/common_headers.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
// #include <pcl/visualization/pcl_visualizer.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>
#include <filesystem>

#include "utils.h"
#include "sfm.h"

#include <tclap/CmdLine.h>
#include <thread>
#include "pcl/common/transforms.h"

using namespace std;

int main(int argc, char* argv[])
{
  TCLAP::CmdLine cmd("Command description message", ' ', "0.9");
  TCLAP::UnlabeledValueArg<std::string> datasetPath("dataset", "", true, "", "dataset");
  TCLAP::UnlabeledValueArg<std::string> resultDirArg("output", "Path to out dir", true, "00", "output");
  TCLAP::ValueArg<std::string> sequenceNumber("s", "sequence", "sequence number", false, "00", "sequence");
  TCLAP::ValueArg<int> startFrameArg("i", "start", "Name to print", false, 0, "start");
  TCLAP::ValueArg<int> endFrameArg("e", "end", "Name to print", false, 0, "end");
  TCLAP::SwitchArg verboseModeArg("v", "verbose", "Print debug info");
  cmd.add(datasetPath);
  cmd.add(sequenceNumber);
  cmd.add(resultDirArg);
  cmd.add(verboseModeArg);
  cmd.add(startFrameArg);
  cmd.add(endFrameArg);
  cmd.parse(argc, argv);

  SFM::Config config;

  config.FEATURES_NUMBER = 2000;
  config.startFrame = startFrameArg.getValue();
  config.endFrame = endFrameArg.getValue();
  config.verbose = verboseModeArg.getValue();

  fs::path sequencesPath = fs::path(datasetPath.getValue()) / fs::path("sequences");
  fs::path sequencePath = sequencesPath / fs::path(sequenceNumber.getValue());

  fs::path imagesPath = sequencePath / fs::path("image_0");
  for (const auto& path : fs::directory_iterator(imagesPath)) {
    config.paths.push_back(path.path());
  }
  std::sort(config.paths.begin(), config.paths.end());
  fs::path calibPath = sequencePath / fs::path("calib.txt");
  config.calibration = getKittiCalibration(calibPath.string());
  auto dds = std::make_unique<CycloneDDS>(config.calibration.extrinsic);

  // create point cloud

  // Camera intristic parameter matrix
  // I did not calibration
  vector<cv::KeyPoint> kpts_vec1, kpts_vec2;
  cv::Mat desc1, desc2;
  cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create();

  cv::Mat image1, image2;

  Eigen::Isometry3f prevPose = Eigen::Isometry3f::Identity();
  Eigen::Isometry3f globalPose = Eigen::Isometry3f::Identity();

  pcl::PointCloud<pcl::PointXYZ>::Ptr globalCloudMap(new pcl::PointCloud<pcl::PointXYZ>);

  cv::Mat K = (cv::Mat_<float>(3, 3) << config.calibration.intrinsic.fx, 0.f, config.calibration.intrinsic.cx, 0.f,
               config.calibration.intrinsic.fy, config.calibration.intrinsic.cy, 0.f, 0.f, 1.f);
  cv::Mat Kd;
  K.convertTo(Kd, CV_64F);

  for (int i = config.startFrame; i < config.endFrame; ++i) {
    if (i == config.startFrame) {
      image1 = cv::imread(config.paths[i].string());
      akaze->detectAndCompute(image1, cv::noArray(), kpts_vec1, desc1);
      continue;
    }
    image2 = cv::imread(config.paths[i].string());

    // extract feature points and calculate descriptors
    akaze->detectAndCompute(image2, cv::noArray(), kpts_vec2, desc2);

    cv::BFMatcher* matcher = new cv::BFMatcher(cv::NORM_L2, false);
    // cross check flag set to false
    // because i do cross-ratio-test match
    vector<vector<cv::DMatch> > matches_2nn_12, matches_2nn_21;
    matcher->knnMatch(desc1, desc2, matches_2nn_12, 2);
    matcher->knnMatch(desc2, desc1, matches_2nn_21, 2);
    const double ratio = 0.8;

    vector<cv::Point2f> selected_points1, selected_points2;
    for (int i = 0; i < matches_2nn_12.size(); i++) {  // i is queryIdx
      if (matches_2nn_12[i][0].distance / matches_2nn_12[i][1].distance < ratio &&
          matches_2nn_21[matches_2nn_12[i][0].trainIdx][0].distance /
                  matches_2nn_21[matches_2nn_12[i][0].trainIdx][1].distance <
              ratio) {
        if (matches_2nn_21[matches_2nn_12[i][0].trainIdx][0].trainIdx == matches_2nn_12[i][0].queryIdx) {
          selected_points1.push_back(kpts_vec1[matches_2nn_12[i][0].queryIdx].pt);
          selected_points2.push_back(kpts_vec2[matches_2nn_21[matches_2nn_12[i][0].trainIdx][0].queryIdx].pt);
        }
      }
    }

    if (true) {
      cv::Mat src;
      cv::hconcat(image1, image2, src);
      for (int i = 0; i < selected_points1.size(); i++) {
        cv::line(src, selected_points1[i], cv::Point2f(selected_points2[i].x + image1.cols, selected_points2[i].y), 1,
                 1, 0);
      }
      cv::imwrite("match-result.png", src);
    }

    cv::Mat mask;  // unsigned char array
    cv::Mat E = cv::findEssentialMat(selected_points1, selected_points2, Kd.at<double>(0, 0),
                                     cv::Point2d(image1.cols / 2., image1.rows / 2.), cv::RANSAC, 0.999, 1.0, mask);
    // E is CV_64F not 32F

    vector<cv::Point2f> inlier_match_points1, inlier_match_points2;
    for (int i = 0; i < mask.rows; i++) {
      if (mask.at<unsigned char>(i)) {
        inlier_match_points1.push_back(selected_points1[i]);
        inlier_match_points2.push_back(selected_points2[i]);
      }
    }
    if (true) {
      cv::Mat src;
      cv::hconcat(image1, image2, src);
      for (int i = 0; i < inlier_match_points1.size(); i++) {
        cv::line(src, inlier_match_points1[i],
                 cv::Point2f(inlier_match_points2[i].x + image1.cols, inlier_match_points2[i].y), 1, 1, 0);
      }
      cv::imwrite("inlier_match_points.png", src);
    }

    mask.release();
    cv::Mat R, t;
    cv::recoverPose(E, inlier_match_points1, inlier_match_points2, R, t, Kd.at<double>(0, 0),
                    cv::Point2d(image1.cols / 2., image1.rows / 2.), mask);
    // R,t is CV_64F not 32F
    R.convertTo(R, CV_32F);
    t.convertTo(t, CV_32F);

    vector<cv::Point2d> triangulation_points1, triangulation_points2;
    for (int i = 0; i < mask.rows; i++) {
      if (mask.at<unsigned char>(i)) {
        triangulation_points1.push_back(
            cv::Point2d((double)inlier_match_points1[i].x, (double)inlier_match_points1[i].y));
        triangulation_points2.push_back(
            cv::Point2d((double)inlier_match_points2[i].x, (double)inlier_match_points2[i].y));
      }
    }

    if (true) {
      cv::Mat src;
      cv::hconcat(image1, image2, src);
      for (int i = 0; i < triangulation_points1.size(); i++) {
        cv::line(src, triangulation_points1[i],
                 cv::Point2f((float)triangulation_points2[i].x + (float)image1.cols, (float)triangulation_points2[i].y),
                 1, 1, 0);
      }
      cv::imwrite("triangulatedPoints.png", src);
    }

    // this shows how a camera moves
    cv::Mat Rinv = R.t();
    cv::Mat T = -Rinv * t;

    Eigen::Isometry3f camRel = Eigen::Isometry3f::Identity();
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        camRel(i, j) = Rinv.at<float>(i, j);
      }
    }
    camRel.translation() = Eigen::Vector3f(T.at<float>(0), T.at<float>(1), T.at<float>(2));

    globalPose = globalPose * camRel;

    cv::Mat Rt0 = cv::Mat::eye(3, 4, CV_64FC1);
    cv::Mat Rt1 = cv::Mat::eye(3, 4, CV_64FC1);

    R.copyTo(Rt1.rowRange(0, 3).colRange(0, 3));
    t.copyTo(Rt1.rowRange(0, 3).col(3));

    cv::Mat point3d_homo;
    cv::triangulatePoints(Kd * Rt0, Kd * Rt1, triangulation_points1, triangulation_points2, point3d_homo);
    // point3d_homo is 64F
    // available input type is here
    // https://stackoverflow.com/questions/16295551/how-to-correctly-use-cvtriangulatepoints

    assert(point3d_homo.cols == triangulation_points1.size());
    std::cout << point3d_homo.cols << std::endl;

    pcl::PointCloud<pcl::PointXYZ>::Ptr localCloud(new pcl::PointCloud<pcl::PointXYZ>);

    for (int i = 0; i < point3d_homo.cols; i++) {
      pcl::PointXYZ point;
      cv::Mat p3d;
      cv::Mat _p3h = point3d_homo.col(i);
      convertPointsFromHomogeneous(_p3h.t(), p3d);
      point.x = p3d.at<double>(0);
      point.y = p3d.at<double>(1);
      point.z = p3d.at<double>(2);

      // std::cout << point.x << " " << point.y << " " << point.z << std::endl;
      localCloud->points.push_back(point);
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr localCloudMap(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::transformPointCloud(*localCloud, *localCloudMap, globalPose.matrix());

    for (int i = 0; i < localCloudMap->points.size(); ++i) {
      globalCloudMap->points.push_back(localCloudMap->points[i]);
    }

    dds->publish(globalCloudMap);

    std::cout << "T " << camRel.translation().transpose().matrix() << " R "
              << camRel.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
    std::cout << "T " << globalPose.translation().transpose().matrix() << " R "
              << globalPose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;

    dds->publish(globalPose.cast<double>(), "odom");
    image1 = image2;
    kpts_vec1 = kpts_vec2;
    desc1 = desc2;
  }

  return 0;
}
