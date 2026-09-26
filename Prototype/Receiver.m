clear all;

% Start TCP server on port 5050 and timeout in 60 seconds
tcpServer = tcpserver("0.0.0.0", 5050, 'Timeout', 60);
disp("Waiting for ESP32 connection...");

while true
    if tcpServer.NumBytesAvailable > 0
        data = readline(tcpServer);
        tokens = split(data, {',', '='});

        if numel(tokens) == 4
            spo2 = str2double(tokens{2});
            temp = str2double(tokens{4});
            fprintf("SPO2: %.1f %% | Temp: %.2f °C\n", spo2, temp);
        else
            fprintf("Malformed data: %s\n", data);
        end
    end
    pause(0.1);
end
