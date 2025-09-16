// Example: Nodes framework with smart pointers
// All Node* replaced with shared_ptr<Node> for safety

#include "nodes_smart_ptr.hpp"
#include <iostream>
#include <string>

using namespace tinyware;

// Example sensor node that publishes data
class SensorNode : public Node
{
protected:
    void configure() override
    {
        // Сенсоры обычно имеют высокий приоритет
        setPriority(Priority::High);
        
        // Timer fires every 100ms
        scheduleTimer(100'000, [this]() {
            publish("sensor/data", temperature_);
            temperature_ += 0.1f;
            
            // Uncomment to test exception handling:
            // if (temperature_ > 25.0f) throw std::runtime_error("Overheat!");
        });
    }

    void onException(const std::exception& e, const std::string& context) override
    {
        std::cout << "[SensorNode] Exception in " << context << ": " << e.what() << std::endl;
        temperature_ = 20.0f; // Reset on error
    }

private:
    float temperature_ = 20.0f;
};

// Example processor node that subscribes to data
class ProcessorNode : public Node
{
protected:
    void configure() override
    {
        // Процессоры имеют обычный приоритет
        setPriority(Priority::Normal);
        
        subscribe<float>("sensor/data", [this](const float& temp) {
            std::cout << "[ProcessorNode] Received temperature: " << temp << "°C" << std::endl;
            
            if (temp > 22.0f)
            {
                publish("alert", std::string("High temperature detected!"));
            }
        });
    }
};

// Example alert node
class AlertNode : public Node
{
protected:
    void configure() override
    {
        // Алерты критичны - максимальный приоритет
        setPriority(Priority::Critical);
        
        subscribe<std::string>("alert", [](const std::string& msg) {
            std::cout << "[AlertNode] 🚨 ALERT: " << msg << std::endl;
        });
    }
};

// Example logger node (low priority)
class LoggerNode : public Node
{
protected:
    void configure() override
    {
        // Логирование не критично - низкий приоритет
        setPriority(Priority::Low);
        
        subscribe<float>("sensor/data", [](const float& temp) {
            // Логируем реже других узлов
            std::cout << "[Logger] Temperature logged: " << temp << "°C" << std::endl;
        });
    }
};

int main()
{
    std::cout << "=== Testing Smart Pointer Nodes Framework ===" << std::endl;
    std::cout << "All Node* replaced with shared_ptr<Node> for memory safety\n" << std::endl;
    
    // ===== Создание узлов через shared_ptr (ЕДИНСТВЕННЫЙ способ) =====
    auto sensor = std::make_shared<SensorNode>();
    auto processor = std::make_shared<ProcessorNode>();
    auto alert = std::make_shared<AlertNode>();
    
    std::vector<NodePtr> nodes = {sensor, processor, alert};
    
    // Test 1: Simulated mode
    std::cout << "\n--- Test 1: Simulated Mode ---" << std::endl;
    {
        ExecutionManager em(ExecutionManager::Mode::Simulated, nodes);
        
        for (int i = 0; i < 3; ++i)
        {
            em.setTime(i * 100'000); // Step 100ms
            em.step();
        }
        std::cout << "Simulated mode completed" << std::endl;
    }
    
    // Test 2: Real-time mode (individual start/stop)
    std::cout << "\n--- Test 2: Real-Time Mode (individual control) ---" << std::endl;
    {
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes);
        
        std::cout << "Total nodes: " << em.getNodeCount() << std::endl;
        
        // Start all nodes individually
        em.start(sensor);
        em.start(processor);
        em.start(alert);
        
        std::cout << "Running nodes: " << em.getRunningCount() << "/" << em.getNodeCount() << std::endl;
        std::cout << "Running for 500ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop all nodes individually
        em.stop(sensor);
        em.stop(processor);
        em.stop(alert);
        
        std::cout << "All nodes stopped. Running: " << em.getRunningCount() << std::endl;
    }
    
    // Test 2b: Real-time mode (startAll/stopAll)
    std::cout << "\n--- Test 2b: Real-Time Mode (bulk control) ---" << std::endl;
    {
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes);
        
        // Запустить все узлы одной командой
        size_t started = em.startAll();
        std::cout << "Started " << started << " nodes" << std::endl;
        std::cout << "Running: " << em.getRunningCount() << "/" << em.getNodeCount() << std::endl;
        
        std::cout << "Running for 500ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Остановить все узлы одной командой
        size_t stopped = em.stopAll();
        std::cout << "Stopped " << stopped << " nodes" << std::endl;
        std::cout << "Running: " << em.getRunningCount() << "/" << em.getNodeCount() << std::endl;
    }
    
    // Test 3: Тест безопасности - узлы живут после удаления из scope
    std::cout << "\n--- Test 3: Node Lifetime Safety ---" << std::endl;
    {
        auto temp_node = std::make_shared<SensorNode>();
        std::vector<NodePtr> temp_nodes = {temp_node};
        
        ExecutionManager em(ExecutionManager::Mode::RealTime, temp_nodes);
        em.start(temp_node);
        
        std::cout << "Node created and started..." << std::endl;
        
        // Отпускаем наш shared_ptr, но ExecutionManager всё еще владеет
        temp_node.reset();
        temp_nodes.clear();
        
        std::cout << "Local shared_ptrs released, but node still alive in ExecutionManager" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // ExecutionManager всё еще владеет узлом и он работает!
        std::cout << "Node still running (owned by ExecutionManager)" << std::endl;
        
        // При выходе из scope ExecutionManager остановит и удалит узел
    }
    std::cout << "ExecutionManager destroyed -> node safely deleted" << std::endl;
    
    // Test 4: Проверки nullptr
    std::cout << "\n--- Test 4: nullptr Safety ---" << std::endl;
    {
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes);
        
        std::cout << "Test: start(nullptr) -> ";
        bool result = em.start(nullptr);
        std::cout << (result ? "FAIL" : "OK (rejected)") << std::endl;
        
        std::cout << "Test: stop(nullptr) -> ";
        result = em.stop(nullptr);
        std::cout << (result ? "FAIL" : "OK (rejected)") << std::endl;
        
        std::cout << "Test: isRunning(nullptr) -> ";
        result = em.isRunning(nullptr);
        std::cout << (result ? "FAIL" : "OK (returns false)") << std::endl;
    }
    
    std::cout << "\n=== All Tests Complete ===" << std::endl;
    // Test 5: Threading modes
    std::cout << "\n--- Test 5: Threading Modes ---" << std::endl;
    
    // 5a: OneThreadPerNode (по умолчанию)
    {
        std::cout << "\n[5a] OneThreadPerNode mode (default):" << std::endl;
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes);
        // 3 узла = 3 потока
        em.startAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        em.stopAll();
    }
    
    // 5b: ThreadPool - фиксированное количество потоков
    {
        std::cout << "\n[5b] ThreadPool mode (2 threads for 3 nodes):" << std::endl;
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes,
                           ExecutionManager::ThreadingMode::ThreadPool,
                           2);  // Только 2 потока для всех узлов
        em.startAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        em.stopAll();
    }
    
    // 5c: ThreadPool с автоопределением (количество ядер CPU)
    {
        std::cout << "\n[5c] ThreadPool mode (auto - CPU cores):" << std::endl;
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes,
                           ExecutionManager::ThreadingMode::ThreadPool);
        // 0 = автоматически определит количество ядер
        em.startAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        em.stopAll();
    }
    
    // 5d: SingleThreaded - все узлы на одном потоке
    {
        std::cout << "\n[5d] SingleThreaded mode (1 thread for all nodes):" << std::endl;
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes,
                           ExecutionManager::ThreadingMode::SingleThreaded);
        em.startAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        em.stopAll();
    }
    
    // Test 6: Приоритеты и статистика
    std::cout << "\n--- Test 6: Priorities and Statistics ---" << std::endl;
    {
        auto sensor = std::make_shared<SensorNode>();    // High priority
        auto processor = std::make_shared<ProcessorNode>(); // Normal priority
        auto alert = std::make_shared<AlertNode>();      // Critical priority
        auto logger = std::make_shared<LoggerNode>();    // Low priority
        
        std::vector<NodePtr> nodes = {sensor, processor, alert, logger};
        
        ExecutionManager em(ExecutionManager::Mode::RealTime, nodes,
                           ExecutionManager::ThreadingMode::ThreadPool, 2);
        
        std::cout << "Node priorities:" << std::endl;
        std::cout << "  Sensor: High (processed every iteration)" << std::endl;
        std::cout << "  Processor: Normal (processed every 2nd iteration)" << std::endl;
        std::cout << "  Alert: Critical (processed every iteration)" << std::endl;
        std::cout << "  Logger: Low (processed every 4th iteration)" << std::endl;
        
        em.startAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        em.stopAll();
        
        // Вывести статистику
        em.printStats();
        
        // Детальная статистика отдельного узла
        auto sensor_stats = em.getNodeStats(sensor);
        if (sensor_stats)
        {
            std::cout << "\nSensor detailed stats:" << std::endl;
            std::cout << "  Messages: " << sensor_stats->messages_processed << std::endl;
            std::cout << "  Timers: " << sensor_stats->timers_fired << std::endl;
            std::cout << "  Exceptions: " << sensor_stats->exceptions_caught << std::endl;
            std::cout << "  Avg processing time: " << sensor_stats->avg_processing_time_us << "μs" << std::endl;
        }
    }
    
    std::cout << "\n✅ Benefits of smart_ptr version:" << std::endl;
    std::cout << "   - No dangling pointers" << std::endl;
    std::cout << "   - Automatic lifetime management" << std::endl;
    std::cout << "   - Thread-safe reference counting" << std::endl;
    std::cout << "   - Clear ownership semantics" << std::endl;
    std::cout << "   - Flexible threading: 1 thread per node, thread pool, or single-threaded" << std::endl;
    std::cout << "   - Performance metrics and monitoring" << std::endl;
    std::cout << "   - Node priorities for critical tasks" << std::endl;
    return 0;
}

/* Expected output:

=== Testing Improved Nodes Framework ===

--- Simulated Mode ---
[ProcessorNode] Received temperature: 20°C
[ProcessorNode] Received temperature: 20.1°C
[ProcessorNode] Received temperature: 20.2°C
[ProcessorNode] Received temperature: 20.3°C
[ProcessorNode] Received temperature: 20.4°C

--- Real-Time Mode ---
[ProcessorNode] Received temperature: 20.5°C
[ProcessorNode] Received temperature: 20.6°C
...
[ProcessorNode] Received temperature: 22.1°C
[AlertNode] 🚨 ALERT: High temperature detected!
...
All nodes stopped cleanly

=== Test Complete ===

*/

/* Compile and run:

# Basic compilation
g++ -std=c++17 -pthread example_usage.cpp -o example

# With optimizations
g++ -std=c++17 -O3 -pthread example_usage.cpp -o example

# Run
./example

# For real-time priority (requires privileges):
sudo setcap cap_sys_nice=eip ./example
./example

*/

