// ======================================================================
// \title  navtestTopology.cpp
// \brief cpp file containing the topology instantiation code
//
// ======================================================================
// Provides access to autocoded functions
#include <navtest/Top/navtestTopologyAc.hpp>
// Note: Uncomment when using Svc:TlmPacketizer
//#include <navtest/Top/navtestPacketsAc.hpp>

// Necessary project-specified types
#include <Fw/Types/MallocAllocator.hpp>
// #include <Os/Log.hpp>
#include <Svc/FramingProtocol/FprimeProtocol.hpp>
#include <Svc/FramingProtocol/DeframingProtocol.hpp>

// Used for 1Hz synthetic cycling
#include <Os/Mutex.hpp>

// Allows easy reference to objects in FPP/autocoder required namespaces
using namespace navtest;

// The reference topology uses a malloc-based allocator for components that need to allocate memory during the
// initialization phase.
Fw::MallocAllocator mallocator;

// The reference topology uses the F´ packet protocol when communicating with the ground and therefore uses the F´
// framing and deframing implementations.
Svc::FprimeFraming framing;
Svc::FprimeDeframing deframing;

Svc::ComQueue::QueueConfigurationTable configurationTable;

// The reference topology divides the incoming clock signal (1Hz) into sub-signals: 1Hz, 1/2Hz, and 1/4Hz with 0 offset
Svc::RateGroupDriver::DividerSet rateGroupDivisorsSet{{{1, 0}, {2, 0}, {4, 0}}};

// Rate groups may supply a context token to each of the attached children whose purpose is set by the project. The
// reference topology sets each token to zero as these contexts are unused in this project.
NATIVE_INT_TYPE rateGroup1Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
NATIVE_INT_TYPE rateGroup2Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};
NATIVE_INT_TYPE rateGroup3Context[Svc::ActiveRateGroup::CONNECTION_COUNT_MAX] = {};

// A number of constants are needed for construction of the topology. These are specified here.
enum TopologyConstants {
    CMD_SEQ_BUFFER_SIZE = 5 * 1024,
    FILE_DOWNLINK_TIMEOUT = 1000,
    FILE_DOWNLINK_COOLDOWN = 1000,
    FILE_DOWNLINK_CYCLE_TIME = 1000,
    FILE_DOWNLINK_FILE_QUEUE_DEPTH = 10,
    HEALTH_WATCHDOG_CODE = 0x123,
    COMM_PRIORITY = 100,
    // bufferManager constants
    FRAMER_BUFFER_SIZE = FW_MAX(FW_COM_BUFFER_MAX_SIZE, FW_FILE_BUFFER_MAX_SIZE + sizeof(U32)) + HASH_DIGEST_LENGTH + Svc::FpFrameHeader::SIZE,
    FRAMER_BUFFER_COUNT = 10,
    DEFRAMER_BUFFER_SIZE = FW_MAX(FW_COM_BUFFER_MAX_SIZE, FW_FILE_BUFFER_MAX_SIZE + sizeof(U32)),
    DEFRAMER_BUFFER_COUNT = 10,
    COM_DRIVER_BUFFER_SIZE = 491520,
    COM_DRIVER_BUFFER_COUNT = 20,
    BUFFER_MANAGER_ID = 200,
    SUBSYSTEMS_DRIVER_BUFFER_SIZE = 491520,
    SUBSYSTEMS_DRIVER_BUFFER_COUNT = 30,
    SUBSYSTEMS_BUFFER_MANAGER_ID = 201
};

// Ping entries are autocoded, however; this code is not properly exported. Thus, it is copied here.
Svc::Health::PingEntry pingEntries[] = {
    {PingEntries::navtest_blockDrv::WARN, PingEntries::navtest_blockDrv::FATAL, "blockDrv"},
    {PingEntries::navtest_tlmSend::WARN, PingEntries::navtest_tlmSend::FATAL, "chanTlm"},
    {PingEntries::navtest_cmdDisp::WARN, PingEntries::navtest_cmdDisp::FATAL, "cmdDisp"},
    {PingEntries::navtest_cmdSeq::WARN, PingEntries::navtest_cmdSeq::FATAL, "cmdSeq"},
    {PingEntries::navtest_eventLogger::WARN, PingEntries::navtest_eventLogger::FATAL, "eventLogger"},
    {PingEntries::navtest_fileDownlink::WARN, PingEntries::navtest_fileDownlink::FATAL, "fileDownlink"},
    {PingEntries::navtest_fileManager::WARN, PingEntries::navtest_fileManager::FATAL, "fileManager"},
    {PingEntries::navtest_fileUplink::WARN, PingEntries::navtest_fileUplink::FATAL, "fileUplink"},
    {PingEntries::navtest_prmDb::WARN, PingEntries::navtest_prmDb::FATAL, "prmDb"},
    {PingEntries::navtest_rateGroup1::WARN, PingEntries::navtest_rateGroup1::FATAL, "rateGroup1"},
    {PingEntries::navtest_rateGroup2::WARN, PingEntries::navtest_rateGroup2::FATAL, "rateGroup2"},
    {PingEntries::navtest_rateGroup3::WARN, PingEntries::navtest_rateGroup3::FATAL, "rateGroup3"},
};

/**
 * \brief configure/setup components in project-specific way
 *
 * This is a *helper* function which configures/sets up each component requiring project specific input. This includes
 * allocating resources, passing-in arguments, etc. This function may be inlined into the topology setup function if
 * desired, but is extracted here for clarity.
 */
void configureTopology(const TopologyState& state) {
    // Buffer managers need a configured set of buckets and an allocator used to allocate memory for those buckets.
    Svc::BufferManager::BufferBins upBuffMgrBins;
    memset(&upBuffMgrBins, 0, sizeof(upBuffMgrBins));
    upBuffMgrBins.bins[0].bufferSize = FRAMER_BUFFER_SIZE;
    upBuffMgrBins.bins[0].numBuffers = FRAMER_BUFFER_COUNT;
    upBuffMgrBins.bins[1].bufferSize = DEFRAMER_BUFFER_SIZE;
    upBuffMgrBins.bins[1].numBuffers = DEFRAMER_BUFFER_COUNT;
    upBuffMgrBins.bins[2].bufferSize = COM_DRIVER_BUFFER_SIZE;
    upBuffMgrBins.bins[2].numBuffers = COM_DRIVER_BUFFER_COUNT;
    bufferManager.setup(BUFFER_MANAGER_ID, 0, mallocator, upBuffMgrBins);

    // Framer and Deframer components need to be passed a protocol handler
    framer.setup(framing);
    deframer.setup(deframing);

    // Command sequencer needs to allocate memory to hold contents of command sequences
    cmdSeq.allocateBuffer(0, mallocator, CMD_SEQ_BUFFER_SIZE);

    // Rate group driver needs a divisor list
    rateGroupDriver.configure(rateGroupDivisorsSet);
    // rateGroupDriver.configure(rateGroupDivisors, FW_NUM_ARRAY_ELEMENTS(rateGroupDivisors)); // form MESMO

    // Rate groups require context arrays.
    rateGroup1.configure(rateGroup1Context, FW_NUM_ARRAY_ELEMENTS(rateGroup1Context));
    rateGroup2.configure(rateGroup2Context, FW_NUM_ARRAY_ELEMENTS(rateGroup2Context));
    rateGroup3.configure(rateGroup3Context, FW_NUM_ARRAY_ELEMENTS(rateGroup3Context));

    // File downlink requires some project-derived properties.
    fileDownlink.configure(FILE_DOWNLINK_TIMEOUT, FILE_DOWNLINK_COOLDOWN, FILE_DOWNLINK_CYCLE_TIME,
                           FILE_DOWNLINK_FILE_QUEUE_DEPTH);

    // Parameter database is configured with a database file name, and that file must be initially read.
    prmDb.configure("PrmDb.dat");
    prmDb.readParamFile();

    // Health is supplied a set of ping entires.
    health.setPingEntries(pingEntries, FW_NUM_ARRAY_ELEMENTS(pingEntries), HEALTH_WATCHDOG_CODE);
    
    // new added
    Svc::BufferManager::BufferBins subsystemsBuffMgrBins;
    memset(&subsystemsBuffMgrBins, 0, sizeof(subsystemsBuffMgrBins));
    subsystemsBuffMgrBins.bins[0].bufferSize = SUBSYSTEMS_DRIVER_BUFFER_SIZE;
    subsystemsBuffMgrBins.bins[0].numBuffers = SUBSYSTEMS_DRIVER_BUFFER_COUNT;
    subsystemsBuffMgrBins.bins[1].bufferSize = SUBSYSTEMS_DRIVER_BUFFER_SIZE;
    subsystemsBuffMgrBins.bins[1].numBuffers = SUBSYSTEMS_DRIVER_BUFFER_COUNT;
    subsystemsBuffMgrBins.bins[2].bufferSize = SUBSYSTEMS_DRIVER_BUFFER_SIZE;
    subsystemsBuffMgrBins.bins[2].numBuffers = SUBSYSTEMS_DRIVER_BUFFER_COUNT;
    subsystemsBuffMgrBins.bins[3].bufferSize = 2048;
    subsystemsBuffMgrBins.bins[3].numBuffers = 20;
    subsystemsBuffMgrBins.bins[4].bufferSize = 2048;
    subsystemsBuffMgrBins.bins[4].numBuffers = 20;
    subsystemsBuffMgrBins.bins[5].bufferSize = 2048;
    subsystemsBuffMgrBins.bins[5].numBuffers = 20;
    subsystemsBuffMgrBins.bins[5].bufferSize = 2048;
    subsystemsBuffMgrBins.bins[5].numBuffers = 20;
    subsystemsFileUplinkBufferManager.setup(SUBSYSTEMS_BUFFER_MANAGER_ID, 0, mallocator, subsystemsBuffMgrBins);

    // Note: Uncomment when using Svc:TlmPacketizer
    // tlmSend.setPacketList(navtestPacketsPkts, navtestPacketsIgnore, 1);

    // Events (highest-priority)
    configurationTable.entries[0] = {.depth = 100, .priority = 0};
    // Telemetry
    configurationTable.entries[1] = {.depth = 500, .priority = 2};
    // File Downlink
    configurationTable.entries[2] = {.depth = 100, .priority = 1};
    // Allocation identifier is 0 as the MallocAllocator discards it
    comQueue.configure(configurationTable, 0, mallocator);
    if (state.hostname != nullptr && state.port != 0) {
        comDriver.configure(state.hostname, state.port);
        // comDriver.startSocketTask(hostname);
    }
}

// Public functions for use in main program are namespaced with deployment name navtest
namespace navtest {
void setupTopology(const TopologyState& state) {
    // Autocoded initialization. Function provided by autocoder.
    initComponents(state);
    // Autocoded id setup. Function provided by autocoder.
    setBaseIds();
    // Autocoded connection wiring. Function provided by autocoder.
    connectComponents();
    // Autocoded configuration. Function provided by autocoder.
    configComponents(state);
    // Deployment-specific component configuration. Function provided above. May be inlined, if desired.
    configureTopology(state);
    // Autocoded command registration. Function provided by autocoder.
    regCommands();
    // Autocoded parameter loading. Function provided by autocoder.
    loadParameters();
    // Autocoded task kick-off (active components). Function provided by autocoder.
    startTasks(state);
    // Initialize socket communication if and only if there is a valid specification
    if (state.hostname != nullptr && state.port != 0) {
        Os::TaskString name("ReceiveTask");
        // Uplink is configured for receive so a socket task is started
        comDriver.start(name, COMM_PRIORITY, Default::STACK_SIZE);
    }

    //GPS
    if (state.gpsComm == nullptr) {
        printf("GPS Comm is null. Defaulting to ttyAMA1\n");
        bool gps_com_open = gps_comm.open("/dev/ttyAMA1", Drv::LinuxUartDriver::BAUD_9600, Drv::LinuxUartDriver::NO_FLOW, Drv::LinuxUartDriver::PARITY_NONE, true);
        printf("GPS Driver Open : %d\n", gps_com_open);
        gps_comm.start();
        printf("GPS start \n");
    }else{
        bool gps_com_open = gps_comm.open(state.gpsComm, Drv::LinuxUartDriver::BAUD_9600, Drv::LinuxUartDriver::NO_FLOW, Drv::LinuxUartDriver::PARITY_NONE, true);
        printf("GPS Driver Open : %d\n", gps_com_open);
        gps_comm.start();
    }
}

// Variables used for cycle simulation
Os::Mutex cycleLock;
volatile bool cycleFlag = true;

void startSimulatedCycle(Fw::TimeInterval interval) {
    cycleLock.lock();
    bool cycling = cycleFlag;
    cycleLock.unLock();

    // Main loop
    while (cycling) {
        navtest::blockDrv.callIsr();
        Os::Task::delay(interval);

        cycleLock.lock();
        cycling = cycleFlag;
        cycleLock.unLock();
    }
}

void stopSimulatedCycle() {
    cycleLock.lock();
    cycleFlag = false;
    cycleLock.unLock();
}

void teardownTopology(const TopologyState& state) {
    // Autocoded (active component) task clean-up. Functions provided by topology autocoder.
    stopTasks(state);
    freeThreads(state);

    // Other task clean-up.
    comDriver.stop();
    (void)comDriver.join();

    // Resource deallocation
    cmdSeq.deallocateBuffer(mallocator);
    bufferManager.cleanup();
    subsystemsFileUplinkBufferManager.cleanup();
}
};  // namespace navtest
