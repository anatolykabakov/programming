#include <iostream>

// Time O(1)
void remove_end(int arr[], int length)
{
  if (length > 0) {
    arr[length - 1] = 0;
  }
}

// Time O(1)
void insert_end(int arr[], int length, int value)
{
  if (length > 0) {
    arr[length - 1] = value;
  }
}

// Time O(n)
void remove_middle(int arr[], int length, int i)
{
  for (int j = i + 1; j < length; ++j) {
    arr[j - 1] = arr[j];
  }
}

// Time O(n)
void insert_middle(int arr[], int length, int i, int value)
{
  for (int j = length - 1; j >= i; --j) {
    arr[j] = arr[j - 1];
  }
  arr[i] = value;
}

void print(int arr[], int length)
{
  for (int i = 0; i < length; ++i) {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;
}

int main()
{
  int arr[] = {1, 2, 3, 4, 5};
  print(arr, 5);
  // Removing a last element of static array takes constant time. O(1).
  remove_end(arr, 5);
  print(arr, 5);
  // Removing a random element from a static array, in the worst case, takes linear time. O(n)
  remove_middle(arr, 5, 0);
  print(arr, 5);
  // Inserting values into end of static array takes constant time. O(1)
  insert_end(arr, 5, 1);
  print(arr, 5);
  // Inserting values into random index of static array takes linear time. O(n)
  insert_middle(arr, 5, 2, 30);
  print(arr, 5);

  return 0;
}
