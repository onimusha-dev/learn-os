#include <iostream>
#include <string>

// ── State enum ──────────────────────────────────────────
// An enum is just a named set of options — clean & readable
enum class ProcessState {
    NEW,        // Just created
    READY,      // Waiting for CPU
    RUNNING,    // Currently on CPU
    WAITING,    // Blocked (e.g. waiting for I/O)
    TERMINATED  // Done
};

// ── CPU Registers snapshot ──────────────────────────────
// When OS pauses a process, it saves registers here
struct CPUContext {
    int program_counter;  // Next instruction address
    int stack_pointer;    // Top of the stack
    int accumulator;      // General purpose register
    // (Real CPUs have many more — x86 has 16+)
};

// ── Memory Info ─────────────────────────────────────────
struct MemoryInfo {
    int base_address;   // Where process starts in RAM
    int limit;          // How much memory it can use
    int heap_pointer;   // Current heap position
};

// ── The PCB itself ──────────────────────────────────────
class PCB {
public:
    // 🔢 Identity
    int         pid;
    int         parent_pid;
    std::string name;

    // 🔄 State — 状態 (joutai)
    ProcessState state;

    // 🧠 CPU Context — saved when process is paused
    CPUContext cpu_context;

    // 💾 Memory — メモリ (memori)
    MemoryInfo memory;

    // ⏱️ Scheduling — 優先度 (yuusendo)
    int priority;           // Higher = more important
    int cpu_time_used;      // Total CPU ms consumed
    int arrival_time;       // When it entered the system

    // ── Constructor ─────────────────────────────────────
    PCB(int pid, std::string name, int parent_pid = 0, int priority = 1)
        : pid(pid),
          name(name),
          parent_pid(parent_pid),
          priority(priority),
          state(ProcessState::NEW),
          cpu_time_used(0),
          arrival_time(0)
    {
        // Zero out context on creation
        cpu_context = {0, 0, 0};
        memory      = {0, 0, 0};
    }

    // ── Helper: print state as string ───────────────────
    std::string getStateString() const {
        switch (state) {
            case ProcessState::NEW:        return "NEW";
            case ProcessState::READY:      return "READY";
            case ProcessState::RUNNING:    return "RUNNING";
            case ProcessState::WAITING:    return "WAITING";
            case ProcessState::TERMINATED: return "TERMINATED";
            default:                       return "UNKNOWN";
        }
    }

    // ── Helper: display the PCB ──────────────────────────
    void display() const {
        std::cout << "====== PCB Info ======\n";
        std::cout << "PID        : " << pid          << "\n";
        std::cout << "Name       : " << name         << "\n";
        std::cout << "Parent PID : " << parent_pid   << "\n";
        std::cout << "State      : " << getStateString() << "\n";
        std::cout << "Priority   : " << priority     << "\n";
        std::cout << "CPU Time   : " << cpu_time_used << "ms\n";
        std::cout << "PC         : " << cpu_context.program_counter << "\n";
        std::cout << "SP         : " << cpu_context.stack_pointer   << "\n";
        std::cout << "Mem Base   : " << memory.base_address         << "\n";
        std::cout << "Mem Limit  : " << memory.limit                << "\n";
        std::cout << "======================\n";
    }
};

int main() {
    // Create a new process — like launching Chrome
    PCB process1(1, "chrome", 0, 5);
    process1.state = ProcessState::READY;
    process1.memory = {1000, 4096, 1200};
    process1.cpu_context.program_counter = 404;

    process1.display();

    // Simulate context switch — save state before pausing
    process1.state = ProcessState::WAITING;
    process1.cpu_context.program_counter = 512; // saved!
    process1.cpu_time_used += 30;

    std::cout << "\nAfter context switch:\n";
    process1.display();

    return 0;
}