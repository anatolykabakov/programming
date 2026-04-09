#include "tracker.h"

TrackerOpenCV::TrackerOpenCV(CameraIntrinsic intrinsic, int featuresNumber)
  : intrinsic_(std::move(intrinsic)), pp_(cv::Point2d(intrinsic.cx, intrinsic.cy)), featuresNumber_(featuresNumber)
{
  detector_ = cv::GFTTDetector::create(featuresNumber, 0.01, 0.0);
}

void TrackerOpenCV::detect()
{
  cv::Mat mask(prevImg.size(), CV_8UC1, cv::Scalar(255));
  int feature_area = (mask.cols * mask.rows) / featuresNumber_;
  int r = std::sqrt(feature_area) / 2;
  for (const auto& p : prevFeatures_) {
    cv::circle(mask, p, r, cv::Scalar(0), cv::FILLED);
  }
  cv::imwrite("mask.png", mask);
  int count = featuresNumber_ - prevFeatures_.size();
  if (count == 0) {
    return;
  }

  detector_ = cv::GFTTDetector::create(count, 0.01, 0.0);

  std::vector<cv::KeyPoint> keypoints;
  detector_->detect(prevImg, keypoints, mask);

  std::vector<cv::Point2f> newPoints;
  cv::KeyPoint::convert(keypoints, newPoints, std::vector<int>());

  for (const auto& p : newPoints) {
    prevFeatures_.push_back(p);
    ++index_;
    indexes_.push_back(index_);
  }
}

void TrackerOpenCV::filter(const cv::Mat& mask)
{
  //  for (int i = 0; i < mask.rows; i++) {
  //     if (mask.at<unsigned char>(i)) {
  //       status[i] = 0;
  //     }
  //  }
  //  fundamentalMFilter(prevFeatures_, currFeatures_, status);
  //  getRidFailedFeatures(prevFeatures_, currFeatures_, status);
}

void TrackerOpenCV::fundamentalMFilter(const std::vector<cv::Point2f>& prev, const std::vector<cv::Point2f>& curr,
                                       std::vector<uchar>& status)
{
  double f_threshold = 1.;
  if (prev.size() <= 8) {
    return;
  }

  std::vector<uchar> f_status;
  cv::findFundamentalMat(prev, curr, cv::FM_RANSAC, f_threshold, 0.99, f_status);

  for (size_t index = 0; index < status.size(); ++index) {
    if (!f_status[index]) {
      status[index] = 0;
    }
  }
}

void TrackerOpenCV::track(const cv::Mat& frame)
{
  try {
    if (prevImg.empty()) {
      prevImg = frame.clone();
    }
    prevFeatures_ = currFeatures_;

    std::cout << status.size() << std::endl;

    if (currFeatures_.size() < featuresNumber_ * 0.8) {
      detect();
    }

    cv::calcOpticalFlowPyrLK(prevImg, frame, prevFeatures_, currFeatures_, status, error, cv::Size(21, 21), 3);

    std::cout << prevFeatures_.size() << std::endl;
    std::cout << status.size() << std::endl;
    // if (prevFeatures_.size())

    // fundamentalMFilter(prevFeatures_, currFeatures_, status);
    spatialFilter(frame, prevFeatures_, currFeatures_, status);
    getRidFailedFeatures(prevFeatures_, currFeatures_, status);

    std::cout << prevFeatures_.size() << std::endl;
    std::cout << status.size() << std::endl;

    prevImg = frame.clone();
  } catch (const std::runtime_error& ex) {
    std::cout << ex.what() << std::endl;
  }
}

void TrackerOpenCV::getRidFailedFeatures(std::vector<cv::Point2f>& points1, std::vector<cv::Point2f>& points2,
                                         std::vector<uchar>& status)
{
  int indexCorrection = 0;
  int N = status.size();
  for (int i = 0; i < N; i++) {
    int k = i - indexCorrection;
    cv::Point2f pt = points2.at(k);
    if ((status.at(k) == 0) || (pt.x < 0) || (pt.y < 0)) {
      if ((pt.x < 0) || (pt.y < 0)) {
        status.at(k) = 0;
      }

      points1.erase(points1.begin() + k);
      points2.erase(points2.begin() + k);
      indexes_.erase(indexes_.begin() + k);
      status.erase(status.begin() + k);

      indexCorrection++;
    }
  }
}

void TrackerOpenCV::spatialFilter(const cv::Mat frame, const std::vector<cv::Point2f>& prev,
                                  const std::vector<cv::Point2f>& curr, std::vector<uchar>& status)
{
  std::vector<double> norms;

  for (int i = 0; i < status.size(); ++i) {
    if (status[i]) {
      const auto norm = std::sqrt(std::pow(curr[i].x - prev[i].x, 2) + std::pow(curr[i].y - prev[i].y, 2));
      norms.push_back(norm);
      const auto threshold = 40.0;
      if (norm > threshold) {
        status[i] = 0;
      }
    }
  }

  std::cout << std::endl;
}
