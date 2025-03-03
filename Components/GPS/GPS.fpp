module Gnc {
    @ GPS for SensorApp
    active component GPS {

        # One async command/port is required for active components
        # This should be overridden by the developers with a useful command/port
        @ A command to force an EVR reporting lock status.
        async command Gps_ReportLockStatus opcode 0

        ###############################################################################
        # User Define Ports:                                                          #
        ###############################################################################

        # sync input port ready: Drv.ByteStreamReady

        sync input port $recv: Drv.ByteStreamRecv

        output port $send: Drv.ByteStreamSend

        output port allocate: Fw.BufferGet

        output port deallocate: Fw.BufferSend

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------
        @ A notification on GPS lock acquired
        event Gps_LockAquired severity activity high id 0 format "GPS lock acquired"

        @ A warning on GPS lock lost
        event Gps_LockLost severity warning high id 1 format "GPS lock lost"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------
        @ The current latitude
        telemetry Gps_Latitude: F32 id 0

        @ The current longitude
        telemetry Gps_Longitude: F32 id 1

        @ The current altitude
        telemetry Gps_Altitude: F32 id 2

        @ The current number of satilites
        telemetry Gps_Count: U32 id 3

    }
}