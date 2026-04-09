#pragma once

#include "utils.h"
#include "tracker.h"
#include "mapping.h"
#include "debug.h"
#include "odom.h"

class SFM {
public:
  struct Config {
    int FEATURES_NUMBER = 1000;
    int startFrame = 0;
    int endFrame = 200;
    double scaleThreshold{0.1};
    bool verbose{false};
    Calibration calibration;

    std::vector<std::filesystem::path> paths;
    std::vector<Eigen::Isometry3d> gt;
  };

  SFM(const SFM::Config& config, std::unique_ptr<IRelativePoseProvider> relativeProvider,
      std::unique_ptr<TrackerOpenCV> monoVO, std::unique_ptr<Odometry> odom, std::shared_ptr<Debug> debug,
      std::shared_ptr<Mapping> pipeline)
    : config_(config)
    , relativeProvider_(std::move(relativeProvider))
    , tracker_(std::move(monoVO))
    , odom_(std::move(odom))
    , debug_(debug)
    , pipeline_(pipeline)
  {
  }

  void run();

private:
  SFM::Config config_;
  std::unique_ptr<TrackerOpenCV> tracker_;
  std::unique_ptr<Odometry> odom_;
  std::shared_ptr<Mapping> pipeline_;
  std::shared_ptr<Debug> debug_;

  std::unique_ptr<IRelativePoseProvider> relativeProvider_;

  Eigen::Isometry3d prevOdom_{Eigen::Isometry3d::Identity()};
};
