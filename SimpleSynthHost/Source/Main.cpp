#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <queue>
#include <mutex>
#include <map>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "ws2_32.lib")

using namespace juce;

// Command-line options parser
struct CommandLineOptions
{
    bool batchMode = false;
    bool stdinMode = false;
    double duration = 0.0;  // 0 = process until stdin closes
    int sampleRate = 44100;
    int blockSize = 512;
    int numChannels = 2;
    std::map<String, float> parameters;  // Parameter name -> value

    static CommandLineOptions parse(int argc, char* argv[])
    {
        CommandLineOptions opts;
        ArgumentList args(argc, argv);

        opts.stdinMode = args.containsOption("--stdin");

        if (args.containsOption("--duration"))
            opts.duration = args.getValueForOption("--duration").getDoubleValue();

        if (args.containsOption("--samplerate"))
            opts.sampleRate = args.getValueForOption("--samplerate").getIntValue();

        // Parse --param arguments
        for (int i = 1; i < args.size(); ++i)
        {
            String arg = args[i].text;
            if (arg.startsWith("--param"))
            {
                String paramSpec = args.getValueForOption(arg);
                int equalsPos = paramSpec.indexOfChar('=');
                if (equalsPos > 0)
                {
                    String name = paramSpec.substring(0, equalsPos).trim();
                    float value = paramSpec.substring(equalsPos + 1).getFloatValue();
                    opts.parameters[name] = value;
                }
            }
        }

        // Auto-detect stdin pipe on Windows
        #ifdef _WIN32
            opts.batchMode = opts.stdinMode || !_isatty(_fileno(stdin));
        #else
            opts.batchMode = opts.stdinMode || !isatty(fileno(stdin));
        #endif

        return opts;
    }
};

// MIDI reader from stdin - raw MIDI bytes
class StdinMidiReader
{
public:
    bool readNextEvent(MidiMessage& msg)
    {
        // Read raw MIDI bytes from stdin
        unsigned char buffer[3];

        // Read status byte
        if (std::cin.read((char*)buffer, 1).gcount() != 1)
            return false;  // EOF or error

        uint8 status = buffer[0];
        int dataBytes = 0;

        // Determine how many data bytes to read
        if ((status & 0xF0) == 0x80 || (status & 0xF0) == 0x90 || (status & 0xF0) == 0xB0)
            dataBytes = 2;
        else if ((status & 0xF0) == 0xC0 || (status & 0xF0) == 0xD0)
            dataBytes = 1;
        else
            return false;  // Unsupported message type

        if (std::cin.read((char*)&buffer[1], dataBytes).gcount() != dataBytes)
            return false;  // Incomplete message

        // Convert to MidiMessage
        if ((status & 0xF0) == 0x90)
            msg = MidiMessage::noteOn((status & 0x0F) + 1, buffer[1], buffer[2] / 127.0f);
        else if ((status & 0xF0) == 0x80)
            msg = MidiMessage::noteOff((status & 0x0F) + 1, buffer[1], buffer[2] / 127.0f);
        else if ((status & 0xF0) == 0xB0)
            msg = MidiMessage::controllerEvent((status & 0x0F) + 1, buffer[1], buffer[2]);

        return true;
    }

    void setNonBlocking()
    {
        // Set stdin to binary mode on Windows
        #ifdef _WIN32
            _setmode(_fileno(stdin), _O_BINARY);
        #endif
    }
};

// Audio writer to stdout - raw float32 PCM
class StdoutAudioWriter
{
public:
    StdoutAudioWriter(int numChannels)
        : channels(numChannels)
    {
        // Set stdout to binary mode on Windows
        #ifdef _WIN32
            _setmode(_fileno(stdout), _O_BINARY);
        #endif
    }

    void write(const AudioBuffer<float>& buffer, int numSamples)
    {
        // Write raw float32 PCM to stdout (interleaved)
        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < channels; ++ch)
            {
                float sample = buffer.getSample(ch, i);
                std::cout.write((const char*)&sample, sizeof(float));
            }
        }
    }

private:
    int channels;
};

// Offline batch renderer - reads MIDI from stdin, writes audio to stdout
class OfflineRenderer
{
public:
    OfflineRenderer(AudioPluginInstance* pluginInstance, const CommandLineOptions& opts)
        : plugin(pluginInstance), options(opts)
    {
    }

    int render()
    {
        if (!plugin)
            return 1;

        // Open debug log file
        FILE* debugLog = fopen("simplesynth_debug.log", "w");

        try
        {
            // Initialize
            if (debugLog) fprintf(debugLog, "[INFO] Starting offline render: %fs at %dHz, blocksize=%d\n",
                                  options.duration, options.sampleRate, options.blockSize);

            // Set up for offline rendering
            plugin->setNonRealtime(true);
            if (debugLog) fprintf(debugLog, "[DEBUG] Set to non-realtime mode\n");

            // Enable all buses (CRITICAL - was missing!)
            plugin->enableAllBuses();
            if (debugLog) fprintf(debugLog, "[DEBUG] All buses enabled\n");

            // Debug: Check bus layout and MIDI capabilities
            auto currentLayout = plugin->getBusesLayout();
            if (debugLog)
            {
                fprintf(debugLog, "[DEBUG] Bus layout: IN=%d buses, OUT=%d buses\n",
                        currentLayout.inputBuses.size(), currentLayout.outputBuses.size());
                fprintf(debugLog, "[DEBUG] acceptsMidi=%d, producesMidi=%d, isMidiEffect=%d\n",
                        plugin->acceptsMidi() ? 1 : 0,
                        plugin->producesMidi() ? 1 : 0,
                        plugin->isMidiEffect() ? 1 : 0);

                // Debug: Check if plugin has MIDI input buses
                int numInputBuses = plugin->getBusCount(false);  // false = input
                int numOutputBuses = plugin->getBusCount(true);  // true = output
                fprintf(debugLog, "[DEBUG] Input buses: %d, Output buses: %d\n", numInputBuses, numOutputBuses);

                for (int i = 0; i < numInputBuses; ++i)
                {
                    auto* bus = plugin->getBus(false, i);
                    if (bus)
                        fprintf(debugLog, "[DEBUG] Input bus %d: layout=%d ch, enabled=%d\n", i, bus->getNumberOfChannels(), bus->isEnabled() ? 1 : 0);
                }
            }

            if (debugLog) fprintf(debugLog, "[DEBUG] Plugin I/O channels: IN=%d OUT=%d\n",
                                  plugin->getTotalNumInputChannels(),
                                  plugin->getTotalNumOutputChannels());

            plugin->prepareToPlay(options.sampleRate, options.blockSize);
            if (debugLog) fprintf(debugLog, "[DEBUG] Plugin prepared for playback\n");
            if (debugLog) fprintf(debugLog, "[DEBUG] After prepare - I/O channels: IN=%d OUT=%d\n",
                                  plugin->getTotalNumInputChannels(),
                                  plugin->getTotalNumOutputChannels());

            // Apply parameters
            int paramsApplied = 0;
            for (const auto& [name, value] : options.parameters)
            {
                for (int i = 0; i < plugin->getNumParameters(); ++i)
                {
                    if (plugin->getParameterName(i) == name)
                    {
                        plugin->setParameter(i, value);
                        if (debugLog) fprintf(debugLog, "[DEBUG] Set parameter: %s = %f\n", name.toRawUTF8(), value);
                        paramsApplied++;
                        break;
                    }
                }
            }
            if (debugLog) fprintf(debugLog, "[DEBUG] Applied %d parameters\n", paramsApplied);

            // Set up I/O
            StdinMidiReader midiReader;
            midiReader.setNonBlocking();
            if (debugLog) fprintf(debugLog, "[DEBUG] MIDI reader initialized (stdin in binary mode)\n");

            StdoutAudioWriter audioWriter(options.numChannels);
            if (debugLog) fprintf(debugLog, "[DEBUG] Audio writer initialized (stdout in binary mode)\n");

            AudioBuffer<float> outputBuffer(options.numChannels, options.blockSize);
            MidiBuffer midiBuffer;


            // Render loop
            int totalSamplesProcessed = 0;
            int maxSamples = (options.duration > 0)
                ? (int)(options.duration * options.sampleRate)
                : 2147483647;  // INT_MAX - process until stdin closes

            bool stdinClosed = false;
            int totalMidiEventsRead = 0;

            int blockNum = 0;
            if (debugLog) fprintf(debugLog, "[DEBUG] Starting render loop (max %d samples)...\n", maxSamples);
            if (debugLog) fflush(debugLog);

            while (totalSamplesProcessed < maxSamples)
            {
                // Read MIDI events for this block (if stdin not closed)
                midiBuffer.clear();
                int eventsThisBlock = 0;
                static MidiMessage sustainNoteOn;  // Keep note on across blocks
                static bool hasSustainNote = false;

                if (!stdinClosed)
                {
                    MidiMessage msg;
                    while (midiReader.readNextEvent(msg))
                    {
                        midiBuffer.addEvent(msg, 0);  // Add at start of block
                        eventsThisBlock++;
                        totalMidiEventsRead++;

                        // Store Note On for sustaining
                        if (msg.isNoteOn())
                        {
                            sustainNoteOn = msg;
                            hasSustainNote = true;
                        }
                        else if (msg.isNoteOff())
                        {
                            hasSustainNote = false;
                        }

                        // Debug first block MIDI events
                        if (blockNum == 0)
                        {
                            if (debugLog)
                            {
                                if (msg.isNoteOn())
                                    fprintf(debugLog, "[DEBUG] MIDI: Note On - note=%d, velocity=%d (added to buffer)\n", msg.getNoteNumber(), msg.getVelocity());
                                else if (msg.isNoteOff())
                                    fprintf(debugLog, "[DEBUG] MIDI: Note Off - note=%d\n", msg.getNoteNumber());
                                else
                                    fprintf(debugLog, "[DEBUG] MIDI: Other message\n");
                            }
                        }
                    }

                    // Debug: log first block with events
                    if (eventsThisBlock > 0 && blockNum == 0)
                        if (debugLog) fprintf(debugLog, "[DEBUG] Block %d: %d MIDI events added to buffer\n", blockNum, eventsThisBlock);

                    // Check if stdin is now closed
                    if (std::cin.eof())
                    {
                        stdinClosed = true;
                        if (debugLog) fprintf(debugLog, "[DEBUG] stdin closed, remaining samples: %d\n", maxSamples - totalSamplesProcessed);
                    }
                }
                else if (hasSustainNote && blockNum < 100)  // Keep sustaining for first 100 blocks after stdin closes
                {
                    // Re-send Note On to keep note playing
                    midiBuffer.addEvent(sustainNoteOn, 0);
                    if (blockNum <= 1)
                        if (debugLog) fprintf(debugLog, "[DEBUG] Re-sending sustained Note On for block %d\n", blockNum);
                }

                // Process audio block (generates audio even without MIDI)
                outputBuffer.clear();

                if (blockNum == 0)
                {
                    if (debugLog) fprintf(debugLog, "[DEBUG] Before processBlock: buffer channels=%d, samples=%d\n",
                                          outputBuffer.getNumChannels(), outputBuffer.getNumSamples());
                    if (debugLog) fprintf(debugLog, "[DEBUG] MidiBuffer size JUST before processBlock: %d events\n", midiBuffer.getNumEvents());

                    // Debug: print MIDI buffer contents
                    if (midiBuffer.getNumEvents() > 0 && debugLog)
                    {
                        fprintf(debugLog, "[DEBUG] MIDI buffer contents:\n");
                        for (auto metadata : midiBuffer)
                        {
                            auto msg = metadata.getMessage();
                            fprintf(debugLog, "  - Note %d, sample %d\n", msg.getNoteNumber(), metadata.samplePosition);
                        }
                    }
                }

                // Process audio block with plugin
                plugin->processBlock(outputBuffer, midiBuffer);

                // Debug: check if we got audio
                if (blockNum == 0 && eventsThisBlock > 0)
                {
                    float maxSample = 0.0f;
                    for (int ch = 0; ch < options.numChannels; ++ch)
                    {
                        maxSample = juce::jmax(maxSample, outputBuffer.getMagnitude(ch, 0, options.blockSize));
                    }
                    if (debugLog) fprintf(debugLog, "[DEBUG] First block with MIDI - max sample: %f\n", maxSample);

                    // Debug: sample a few values
                    if (debugLog)
                    {
                        auto* ch0 = outputBuffer.getReadPointer(0);
                        fprintf(debugLog, "[DEBUG] First 10 samples ch0: ");
                        for (int s = 0; s < juce::jmin(10, outputBuffer.getNumSamples()); ++s)
                            fprintf(debugLog, "%f ", ch0[s]);
                        fprintf(debugLog, "\n");
                    }
                }

                // Write to stdout
                audioWriter.write(outputBuffer, options.blockSize);

                totalSamplesProcessed += options.blockSize;
                blockNum++;

                // Log progress every 100 blocks (about every 1 second at 44100 Hz, 512 blocksize)
                if (blockNum % 100 == 0)
                    if (debugLog) fprintf(debugLog, "[DEBUG] Block %d, samples: %d/%d\n", blockNum, totalSamplesProcessed, maxSamples);
            }

            if (debugLog) fprintf(debugLog, "[DEBUG] Render loop completed. Total MIDI events: %d, blocks: %d\n", totalMidiEventsRead, blockNum);

            // Cleanup
            plugin->releaseResources();
            plugin->setNonRealtime(false);
            if (debugLog) fprintf(debugLog, "[DEBUG] Cleanup complete\n");
            if (debugLog) fclose(debugLog);

            return 0;
        }
        catch (const std::exception& e)
        {
            if (debugLog) fprintf(debugLog, "ERROR: %s\n", e.what());
            if (debugLog) fclose(debugLog);
            return 1;
        }
    }

private:
    AudioPluginInstance* plugin;
    CommandLineOptions options;
};

// UDP MIDI Receiver - listens for MIDI messages from Python bridge
class UDPMIDIReceiver
{
public:
    UDPMIDIReceiver(MidiMessageCollector& collector) : midiCollector(collector)
    {
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~UDPMIDIReceiver()
    {
        stop();
        WSACleanup();
    }

    bool start(int port = 9999)
    {
        socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket == INVALID_SOCKET)
        {
            std::cout << "[ERROR] Failed to create UDP socket" << std::endl;
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (::bind(socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            std::cout << "[ERROR] Failed to bind UDP socket to port " << port << std::endl;
            closesocket(socket);
            return false;
        }

        running = true;
        receiverThread = std::thread(&UDPMIDIReceiver::receiveLoop, this);
        std::cout << "[*] UDP MIDI receiver started on port " << port << std::endl;
        return true;
    }

    void stop()
    {
        running = false;
        if (socket != INVALID_SOCKET)
        {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
        if (receiverThread.joinable())
            receiverThread.join();
    }

private:
    MidiMessageCollector& midiCollector;
    WSADATA wsaData;
    SOCKET socket = INVALID_SOCKET;
    bool running = false;
    std::thread receiverThread;

    void receiveLoop()
    {
        unsigned char buffer[3];
        sockaddr_in fromAddr;
        int fromAddrLen = sizeof(fromAddr);

        while (running)
        {
            int bytesReceived = recvfrom(socket, (char*)buffer, sizeof(buffer), 0,
                                        (sockaddr*)&fromAddr, &fromAddrLen);

            if (bytesReceived == 3)
            {
                // Parse MIDI message
                uint8 status = buffer[0];
                uint8 data1 = buffer[1];
                uint8 data2 = buffer[2];

                MidiMessage msg;

                // Determine message type from status byte
                if ((status & 0xF0) == 0x90)  // Note On
                {
                    msg = MidiMessage::noteOn((status & 0x0F) + 1, data1, data2 / 127.0f);
                }
                else if ((status & 0xF0) == 0x80)  // Note Off
                {
                    msg = MidiMessage::noteOff((status & 0x0F) + 1, data1, data2 / 127.0f);
                }
                else if ((status & 0xF0) == 0xB0)  // Control Change
                {
                    msg = MidiMessage::controllerEvent((status & 0x0F) + 1, data1, data2);
                }
                else
                {
                    continue;  // Skip unsupported message types
                }

                // Add to MIDI collector
                midiCollector.addMessageToQueue(msg);
            }
            else if (bytesReceived == SOCKET_ERROR)
            {
                if (running)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
};

// Interactive host with UDP MIDI support
class SimpleSynthHost
{
public:
    SimpleSynthHost(std::unique_ptr<AudioPluginInstance> pluginInstance)
        : plugin(std::move(pluginInstance))
    {
    }

    bool initialise()
    {
        try
        {
            std::cout << "===== SimpleSynth Host =====" << std::endl;
            std::cout << "Initializing audio device..." << std::endl;

            // Setup audio device with explicit sample rate and buffer size
            AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup(setup);
            setup.sampleRate = 44100.0;
            setup.bufferSize = 512;

            String error = deviceManager.initialise(0, 2, nullptr, true, {}, &setup);

            if (error.isNotEmpty())
            {
                std::cout << "ERROR: Could not initialize audio device!" << std::endl;
                std::cout << "Details: " << error << std::endl;
                return false;
            }

            std::cout << "Audio device initialized successfully!" << std::endl;

            // Get current audio device info
            if (auto* device = deviceManager.getCurrentAudioDevice())
            {
                std::cout << "Using audio device: " << device->getName() << std::endl;
                std::cout << "Sample rate: " << device->getCurrentSampleRate() << " Hz" << std::endl;
                std::cout << "Buffer size: " << device->getCurrentBufferSizeSamples() << " samples" << std::endl;
            }

            // Connect audio player to audio device
            deviceManager.addAudioCallback(&player);
            std::cout << "Audio player connected to device." << std::endl;

            // List and enable all MIDI inputs
            std::cout << "\nAvailable MIDI inputs:" << std::endl;
            auto midiInputs = MidiInput::getAvailableDevices();
            if (midiInputs.isEmpty())
            {
                std::cout << "  (none found)" << std::endl;
            }
            else
            {
                for (auto& input : midiInputs)
                {
                    std::cout << "  - " << input.name << std::endl;
                    // Enable this MIDI input device
                    deviceManager.setMidiInputDeviceEnabled(input.identifier, true);
                }
            }

            // Connect all MIDI inputs to the player
            deviceManager.addMidiInputDeviceCallback({}, &player);
            std::cout << "MIDI input connected." << std::endl;

            if (!plugin)
            {
                std::cout << "ERROR: No plugin provided!" << std::endl;
                return false;
            }

            // Enable all buses
            plugin->enableAllBuses();

            // Connect plugin to player
            player.setProcessor(plugin.get());
            std::cout << "Plugin connected to audio player." << std::endl;

            // Print plugin parameters
            int numParams = plugin->getNumParameters();
            std::cout << "\nPlugin parameters (" << numParams << " total):" << std::endl;
            for (int i = 0; i < juce::jmin(numParams, 10); ++i)
            {
                auto paramName = plugin->getParameterName(i);
                auto paramValue = plugin->getParameter(i);
                std::cout << "  " << i << ": " << paramName << " = " << paramValue << std::endl;
            }

            // Setup UDP MIDI receiver for Python bridge
            std::cout << "\nStarting UDP MIDI receiver..." << std::endl;
            udpMidiReceiver = std::make_unique<UDPMIDIReceiver>(midiCollector);
            if (!udpMidiReceiver->start(9999))
            {
                std::cout << "WARNING: UDP MIDI receiver failed to start" << std::endl;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception during initialization: " << e.what() << std::endl;
            return false;
        }
    }

    void shutdown()
    {
        try
        {
            std::cout << "\nShutting down..." << std::endl;

            // Stop audio callbacks in correct order
            deviceManager.removeAudioCallback(&player);
            deviceManager.removeMidiInputDeviceCallback({}, &player);

            // Clear processor before destroying plugin
            player.setProcessor(nullptr);

            // Destroy plugin
            plugin.reset();

            std::cout << "Shutdown complete." << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception during shutdown: " << e.what() << std::endl;
        }
    }

    int run()
    {
        std::cout << "========================================" << std::endl;
        std::cout << "SimpleSynth is ready!" << std::endl;
        std::cout << "Send MIDI notes to play the synth." << std::endl;
        std::cout << "Press Ctrl+C to exit." << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Keep running until interrupted
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return 0;
    }

private:
    AudioDeviceManager deviceManager;
    AudioPluginFormatManager formatManager;
    AudioProcessorPlayer player;
    std::unique_ptr<AudioPluginInstance> plugin;
    std::unique_ptr<UDPMIDIReceiver> udpMidiReceiver;
    MidiMessageCollector midiCollector;
};

// Helper function to load SimpleSynth VST3 plugin
std::unique_ptr<AudioPluginInstance> loadSimpleSynthPlugin(int sampleRate, int blockSize)
{
    AudioPluginFormatManager formatManager;
    formatManager.addFormat(new VST3PluginFormat());

    String cwd = File::getCurrentWorkingDirectory().getFullPathName();
    String pluginPath = cwd + "/SimpleSynth/cmake-build/SimpleSynth_artefacts/Debug/VST3/SimpleSynth.vst3";
    File vst3File(pluginPath);

    // VST3 is a directory bundle, check if directory exists
    if (!vst3File.exists() || !vst3File.isDirectory())
    {
        std::cerr << "ERROR: Plugin file not found at: " << vst3File.getFullPathName() << std::endl;
        return nullptr;
    }

    // Discover plugins in the file
    OwnedArray<PluginDescription> pluginDescriptions;
    VST3PluginFormat vst3Format;
    vst3Format.findAllTypesForFile(pluginDescriptions, vst3File.getFullPathName());

    if (pluginDescriptions.isEmpty())
    {
        std::cerr << "ERROR: No VST3 plugins found in file!" << std::endl;
        return nullptr;
    }

    // Load plugin synchronously
    String loadError;
    auto plugin = formatManager.createPluginInstance(
        *pluginDescriptions[0],
        sampleRate,
        blockSize,
        loadError);

    if (!plugin)
    {
        std::cerr << "ERROR: Failed to load plugin: " << loadError << std::endl;
        return nullptr;
    }

    return plugin;
}

// Main entry point
int main(int argc, char* argv[])
{
    // Parse command-line options
    CommandLineOptions opts = CommandLineOptions::parse(argc, argv);

    // Load SimpleSynth plugin
    auto plugin = loadSimpleSynthPlugin(opts.sampleRate, opts.blockSize);
    if (!plugin)
    {
        std::cerr << "Failed to load SimpleSynth plugin." << std::endl;
        return 1;
    }

    // Choose mode
    if (opts.batchMode)
    {
        // Batch mode - stdin/stdout test harness
        std::cerr << "[SimpleSynthHost] Batch mode" << std::endl;
        OfflineRenderer renderer(plugin.get(), opts);
        return renderer.render();
    }
    else
    {
        // Interactive mode - UDP MIDI receiver
        SimpleSynthHost host(std::move(plugin));

        if (!host.initialise())
        {
            std::cout << "Failed to initialize host." << std::endl;
            return 1;
        }

        try
        {
            host.run();
        }
        catch (const std::exception& e)
        {
            std::cout << "Exception during run: " << e.what() << std::endl;
        }

        host.shutdown();
        return 0;
    }
}
