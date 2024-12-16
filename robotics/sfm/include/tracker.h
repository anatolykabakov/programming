#pragma once

#include "utils.h"

class TrackerOpenCV {
public:
  TrackerOpenCV(CameraIntrinsic intrinsic, int featuresThreshold);

  void track(const cv::Mat& frame);

  void filter(const cv::Mat& mask);

  std::vector<cv::Point2f> prevFeatures() { return prevFeatures_; }

  std::vector<cv::Point2f> currFeatures() { return currFeatures_; }

  std::vector<unsigned char> trackingStatus() { return status; }

  std::vector<int> indexes() { return indexes_; }

private:
  std::vector<cv::Point2f> prevFeatures_;
  std::vector<cv::Point2f> currFeatures_;
  std::vector<int> indexes_;
  std::vector<unsigned char> status;
  std::vector<float> error;
  cv::Mat prevImg;
  double focal_;
  cv::Point2d pp_;
  CameraIntrinsic intrinsic_;

  cv::Ptr<cv::FeatureDetector> detector_;
  double scaleThreshold = 0.1;
  int featuresNumber_{1000};
  int index_{0};

  void detect();

  void spatialFilter(const cv::Mat frame, const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& curr,
                     std::vector<uchar>& status);

  void fundamentalMFilter(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& curr,
                          std::vector<uchar>& status);

  void getRidFailedFeatures(std::vector<cv::Point2f>& points1, std::vector<cv::Point2f>& points2,
                            std::vector<uchar>& status);
};
