#include "visual_odom_mono.h"

#include <tclap/CmdLine.h>

int main(int argc, char** argv)
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

  const auto first = cv::imread(config.paths[config.startFrame].string());

  fs::path calibPath = sequencePath / fs::path("calib.txt");
  config.calibration = getKittiCalibration(calibPath.string());
  config.calibration.intrinsic.width = first.size().width;
  config.calibration.intrinsic.height = first.size().height;

  fs::path gtPath = fs::path(datasetPath.getValue()) / fs::path("poses") / fs::path(sequenceNumber.getValue());
  gtPath.replace_extension(".txt");
  config.gt = parseGTFile(gtPath.string());

  auto odom = std::make_unique<Odometry>(config.gt[config.startFrame]);

  auto debug = std::make_shared<Debug>(config.calibration.extrinsic);

  std::unique_ptr<IRelativePoseProvider> relativeProvider =
      std::make_unique<RelativePoseProviderOpenCV>(config.calibration.intrinsic);
  // std::unique_ptr<IRelativePoseProvider> relativeProvider = std::make_unique<RelativePoseProviderOpenMVG>(intrinsic);

  auto monoVO = std::make_unique<TrackerOpenCV>(config.calibration.intrinsic, config.FEATURES_NUMBER);

  auto pipeline = std::make_shared<Pipeline>(config.calibration.intrinsic);

  auto sfm =
      std::make_unique<SFM>(config, std::move(relativeProvider), std::move(monoVO), std::move(odom), debug, pipeline);
  sfm->run();

  fs::path trajectoryImgPath = fs::path(resultDirArg.getValue()) / fs::path("trajectory.png");
  debug->saveImg(trajectoryImgPath);

  fs::path sceneBeforePath = fs::path(resultDirArg.getValue()) / fs::path("scene.ply");
  // pipeline->adjust();
  pipeline->save(sceneBeforePath);
}
