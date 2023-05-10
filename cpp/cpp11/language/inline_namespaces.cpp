namespace Program {
namespace Version1 {
int getVersion() { return 1; }
bool isFirstVersion() { return true; }
}  // namespace Version1
inline namespace Version2 {
int getVersion() { return 2; }
}  // namespace Version2
}  // namespace Program
int main() {
    int version{Program::getVersion()};               // Uses getVersion() from Version2
    int oldVersion{Program::Version1::getVersion()};  // Uses getVersion() from Version1
    // bool firstVersion {Program::isFirstVersion()};    // Does not compile when Version2
    // is added
}
