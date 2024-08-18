#include "visual_odom_mono.h"

#include <tclap/CmdLine.h>

int main(int argc, char** argv)
{
  TCLAP::CmdLine cmd("Command description message", ' ', "0.9");
  TCLAP::UnlabeledValueArg<std::string> datasetPath("dataset", "", true, "", "dataset");
  TCLAP::UnlabeledValueArg<std::string> resultDirArg("output", "Path to out dir", true, "00", "output");
  TCLAP::ValueArg<std::string> sequenceNumber("s", "sequence", "sequence number", false, "00", "sequence");
  TCLAP::ValueArg<int> startFrameArg("i", "start", "Name to print", false, 0, "start");
  TCLAP::SwitchArg verboseModeArg("v", "verbose", "Print debug info");
  cmd.add(datasetPath);
  cmd.add(sequenceNumber);
  cmd.add(resultDirArg);
  cmd.add(verboseModeArg);
  cmd.add(startFrameArg);
  cmd.parse(argc, argv);

  bool verbose = verboseModeArg.getValue();

  int FEATURES_NUMBER = 1000;
  int offset = 500;
  double imgScale = 1.3;
  int startFrame = startFrameArg.getValue();

  fs::path sequencesPath = fs::path(datasetPath.getValue()) / fs::path("sequences");
  fs::path sequencePath = sequencesPath / fs::path(sequenceNumber.getValue());

  fs::path imagesPath = sequencePath / fs::path("image_0");
  std::vector<std::filesystem::path> paths;
  for (const auto& path : fs::directory_iterator(imagesPath)) {
    paths.push_back(path.path());
  }
  std::sort(paths.begin(), paths.end());

  const auto first = cv::imread(paths[startFrame].string());

  fs::path calibPath = sequencePath / fs::path("calib.txt");
  auto intrinsic = getKittiCameraIntrinsic(calibPath.string());
  intrinsic.width = first.size().width;
  intrinsic.height = first.size().height;

  fs::path gtPath = fs::path(datasetPath.getValue()) / fs::path("poses") / fs::path(sequenceNumber.getValue());
  gtPath.replace_extension(".txt");
  const auto gtPositions = parseGTFile(gtPath.string());

  const auto white = cv::Scalar(255, 255, 255);
  const auto red = cv::Scalar(0, 0, 255);
  const auto blue = cv::Scalar(255, 0, 0);
  const auto green = cv::Scalar(0, 255, 0);

  auto odom = std::make_unique<Odometry>(gtPositions[startFrame]);
  double scaleThreshold{0.1};

  // std::unique_ptr<IRelativePoseProvider> relativeProvider = std::make_unique<RelativePoseProviderOpenCV>(intrinsic);
  std::unique_ptr<IRelativePoseProvider> relativeProvider = std::make_unique<RelativePoseProviderOpenMVG>(intrinsic);

  cv::Mat trajectoryImg = cv::Mat(1200, 1200, CV_8UC3, white);

  auto monoVO = std::make_unique<MonoVO>(intrinsic, std::move(relativeProvider), FEATURES_NUMBER);

  for (int i = startFrame + 1; i < paths.size(); ++i) {
    const auto frame = cv::imread(paths[i].string());

    auto relativePose = monoVO->handle(frame);

    double scale = getOdomScale(gtPositions[i].translation(), gtPositions[i - 1].translation());
    if (scale > scaleThreshold && relativePose.translation().z() > relativePose.translation().x() &&
        relativePose.translation().z() > relativePose.translation().y()) {
      relativePose.translation() *= scale;
      odom->update(relativePose);
    }

    const auto globalPose = odom->pose();

    const auto posePoint = convertToImagePoint(globalPose, trajectoryImg.size().height, offset, imgScale);
    const auto gtPoint = convertToImagePoint(gtPositions[i], trajectoryImg.size().height, offset, imgScale);

    cv::circle(trajectoryImg, posePoint, 1, green, 2);
    cv::circle(trajectoryImg, gtPoint, 1, red, 2);

    if (verbose) {
      const auto gtRelative = gtPositions[i - 1].inverse() * gtPositions[i];
      std::cout << "flame: " << i << std::endl;
      std::cout << "scale: " << scale << std::endl;
      std::cout << "rel norm: " << relativePose.translation().norm() << std::endl;
      // std::cout << "currFeatures size " << currFeatures.size() << std::endl;
      std::cout << "gtRelative t " << gtRelative.translation().transpose().matrix() << " R "
                << gtRelative.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
      std::cout << "relativePose t " << relativePose.translation().transpose().matrix() << " R "
                << relativePose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
      std::cout << "globalPose t " << globalPose.translation().transpose().matrix() << " R "
                << globalPose.linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
      std::cout << "gtPositions t " << gtPositions[i].translation().transpose().matrix() << " R "
                << gtPositions[i].linear().eulerAngles(0, 1, 2).transpose().matrix() << std::endl;
      std::cout << std::endl;
      cv::imshow("trajectoryImg", trajectoryImg);
      cv::imshow("currentImg", frame);
      cv::waitKey(0);
    } else {
      progress_bar(i, paths.size());
    }
  }
  fs::path trajectoryImgPath = fs::path(resultDirArg.getValue()) / fs::path("trajectory.png");
  cv::imwrite(trajectoryImgPath, trajectoryImg);
}
