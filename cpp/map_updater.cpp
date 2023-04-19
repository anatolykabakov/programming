#include <pcl/point_cloud.h>
#include <pcl/kdtree/kdtree_flann.h>

#include <iostream>
#include <vector>
#include <ctime>
#include <unordered_map>


enum class ClassificationResult {
  TP,
  TN,
  FN,
  FP
};

std::unordered_map<ClassificationResult, std::string> association = {{ClassificationResult::TP, "TP"}, {ClassificationResult::TN, "TN"}, {ClassificationResult::FP, "FP"}, {ClassificationResult::FN, "FN"}};

pcl::PointCloud<pcl::PointXYZ>::Ptr foo(pcl::PointCloud<pcl::PointXYZ>::Ptr map, pcl::PointCloud<pcl::PointXYZ>::Ptr scan, float threshold = 1.0) {
  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_map;
  kdtree_map.setInputCloud(map);

  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_scan;
  kdtree_scan.setInputCloud(scan);

  std::vector<ClassificationResult> m_map(map->points.size(), ClassificationResult::FP);
  std::vector<ClassificationResult> m_scan(scan->points.size(), ClassificationResult::FN);

  for (int i = 0; i < scan->points.size(); ++i) {
    auto& point = scan->points[i];
    std::vector<int> inds(1, -1);
    std::vector<float> dists(1, -1.0);
    int k = kdtree_map.radiusSearch(point, 1, inds, dists);
    if (k > 0 && dists[0] <= threshold) {
      m_scan[i] = ClassificationResult::TP;
    }
  }

  for (int i = 0; i < map->points.size(); ++i) {
    auto& point = map->points[i];
    std::vector<int> inds(1, -1);
    std::vector<float> dists(1, -1.0);
    int k = kdtree_scan.radiusSearch(point, 1, inds, dists);
    if (k > 0 && dists[0] <= threshold) {
      m_map[i] = ClassificationResult::TP;
    }
  }

  pcl::PointCloud<pcl::PointXYZ>::Ptr new_map(new pcl::PointCloud<pcl::PointXYZ>);

  for (int i=0; i<map->points.size(); ++i) {
    if (m_map[i] == ClassificationResult::TP) {
      new_map->points.push_back(map->points[i]);
    }
  }

  for (int i=0; i<scan->points.size(); ++i) {
    if (m_scan[i] == ClassificationResult::FN) {
      new_map->points.push_back(scan->points[i]);
    }
  }

  return new_map;
}

void testcase1() {
  std::cout << "testcase 1" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));
  
  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i=0; i<result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl; 
  }
}

void testcase2() {
  std::cout << "testcase 2" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));
  
  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i=0; i<result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl; 
  }
}

void testcase3() {
  std::cout << "testcase 3" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));
  
  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i=0; i<result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl; 
  }
}

int main (int argc, char** argv)
{
  testcase1();
  testcase2();
  testcase3();

  return 0;
}