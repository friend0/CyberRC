javaaddpath('./cyberrc/RCDataOuterClass.jar');

classdef CyberRCControllerWriter < handle
    properties
        port
        baudRate = 921600
    end
    
    methods
        function obj = CyberRCControllerWriter(port, baudRate)
            if nargin > 0
                obj.port = port;
            end
            if nargin > 1
                obj.baudRate = baudRate;
            end
        end
        
        function run(obj)
            fprintf('CyberRC Controller Writer\n');
            if isempty(obj.port)
                ports = serialportlist("available");
                if isempty(ports)
                    error('Failed to list available ports');
                end
                while true
                    fprintf('\nSelect an available port:\n');
                    for i = 1:length(ports)
                        fprintf('%d: %s\n', i, ports{i});
                    end
                    choice = input('Enter the number of the port: ', 's');
                    choice = str2double(choice);
                    if ~isnan(choice) && choice >= 1 && choice <= length(ports)
                        obj.port = ports{choice};
                        break;
                    else
                        fprintf('Invalid choice. Please try again.\n');
                    end
                end
            end
            
            % Open the serial port
            s = serialport(obj.port, obj.baudRate);
            configureTerminator(s, "LF");
            fprintf('Connected to %s at %d baud rate\n', obj.port, obj.baudRate);
            
            % Example of encoding data using protobufs
            % Assuming you have a compiled protobuf class named 'RcData'
            data = RcData();
            data.aileron = 0.5;
            data.elevator = 0.1;
            data.throttle = 0.8;
            data.rudder = 0.2;
            
            % Serialize the data to binary
            encodedData = serialize(data);
            
            % Send the encoded data over the serial port
            write(s, encodedData, "uint8");
            
            % Close the serial port
            clear s;
        end
    end
end

% Usage example:
% writer = CyberRCControllerWriter();
% writer.run();