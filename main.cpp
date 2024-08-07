#include <QCoreApplication>
#include <QThread>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QElapsedTimer>

#include <functional>
#include <memory>
#include <fstream>
#include <iostream>
#include <stdio.h>

static void usage(){
    std::cerr << "Usage:" << std::endl;
    std::cerr << QCoreApplication::applicationName().toStdString() << " <SERIAL PORT> <RESULT.TXT> [BAUD RATES]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Example:" << std::endl;
    std::cerr << "    Windows -> " << QCoreApplication::applicationName().toStdString() << " COM1" << std::endl;
    std::cerr << "      Linux -> " << QCoreApplication::applicationName().toStdString() << " /dev/ttyS0" << std::endl;
}

static  std::function<void(int, char**)> main_thread =
[](int argc, char** argv)
{
    //Parse command-line options
    if(argc < 2){
        std::cerr << "[ERR] Missing serial port argument" << std::endl;
        usage();
        QCoreApplication::exit(0);
        return;
    }
    if((QString(argv[1]) == "--help") || (QString(argv[1]) == "-h")){
        usage();
        QCoreApplication::exit(0);
        return;
    }

    //Show  some info on the selected port
    QSerialPortInfo portInfo(argv[1]);
    if(portInfo.isNull()){
        std::cerr << "[ERR] Unable to get <" << argv[1] << "> port info" << std::endl;
        QCoreApplication::exit(0);
        return;
    }
    std::cout << portInfo.portName().toStdString() << " (" << portInfo.description().toStdString() << ")" << std::endl;
    std::cout << "Vendor: " << portInfo.manufacturer().toStdString() << std::endl;

    std::ofstream fout(argv[2]);
    if (!fout.is_open()) {
        std::cerr << "[ERR] Unable to open file: " << argv[2] << std::endl;
        QCoreApplication::exit(0);
        return;
    }

    // Parse baud rates from third argument (separated with ",")
    std::vector<qint32> baud_rates;
    if (argc > 3) {
        std::string baud_rates_str(argv[3]);
        size_t pos = 0;
        std::string token;
        while ((pos = baud_rates_str.find(",")) != std::string::npos) {
            token = baud_rates_str.substr(0, pos);
            baud_rates.push_back(std::stoi(token));
            baud_rates_str.erase(0, pos + 1);
        }
        baud_rates.push_back(std::stoi(baud_rates_str));
    } else {
        for (const qint32 rate : QSerialPortInfo::standardBaudRates()) {
            baud_rates.push_back(rate);
        }
    }

    //Time divisor (use 1 for nanoseconds)
    const uint64_t time_div = 1000;
    if (time_div == 1) {
        std::cout << "Time shown in ns" <<std::endl;
    }
    else if (time_div == 1000){
        std::cout << "Time shown in ms" <<std::endl;
    }
    else{
        std::cout << "Time shown in s/10E+" << time_div << std::endl;
    }

    //Start a reference timer
    QElapsedTimer timer;
    timer.start();

    //Print table header
    printf("% 7s  % 10s % 10s % 10s % 10s % 10s\r\n", "Speed", "Ideal", "Minimum", "Average", "Maximum", "Delta");
    //Perform test on selected port for each baudrate
    std::map<qint32, std::vector<uint64_t>> result_map;
    for (qint32 baudrate : baud_rates){
        QSerialPort serialPort(portInfo);
        serialPort.setDataBits(QSerialPort::Data8);
        serialPort.setStopBits(QSerialPort::OneStop);
        serialPort.setParity(QSerialPort::NoParity);
        serialPort.setBaudRate(baudrate);

        //Flush stdout since some shells (bash on windows) are a bit picky
        fflush(stdout);

        //Calculate expected minimum delay as the time it takes to send
        //one byte and receive one byte
        uint64_t minimum_delay_ns = (1000000000LL * 10LL) / baudrate;

        //Print baudrate
        printf("% 7d: % 10llu ", baudrate, minimum_delay_ns / time_div);

        //Try opening the port
        if(!serialPort.open(QIODevice::ReadWrite)){
            serialPort.close();
            printf("(Unsupported)\r\n");
            continue;
        }

        //Clear read buffer just in case
        {
            char dummy[1024];
            serialPort.read(dummy, sizeof(dummy));
        }

        //send N individual bytes over the wire and calculate minimum, maximum and average delays
        const size_t N = 5000;
        uint8_t b = 0;
        uint64_t delay_max = 0;
        uint64_t delay_min = UINT64_MAX;
        uint64_t delay_avg = 0;
        bool has_timed_out = false;
        bool has_errors = false;
        std::vector<uint64_t> sample_array;
        sample_array.resize(N);
        for (int i = 0; i < N; i++) {
            qint64 start_time = timer.nsecsElapsed();
            uint8_t read_byte;

            //Flush stdout again so baudrate is shown
            fflush(stdout);

            serialPort.putChar(b);
            if(serialPort.waitForReadyRead(100)){
                //There is data to read, All OK
                ;
            }
            else{
                //No data ready. Is loopback cable installed?
                has_timed_out = true;
            }
            serialPort.read((char*)&read_byte, 1);
            if(read_byte != b){
                has_errors = true;
            }
            qint64 sample_time = timer.nsecsElapsed() - start_time;

            //Update timings
            if(sample_time > delay_max) delay_max = sample_time;
            if(sample_time < delay_min) delay_min = sample_time;
            delay_avg += sample_time;
            sample_array[i] = sample_time;

            b++;
        }while(b != 0);
        delay_avg = delay_avg / N;
        serialPort.close();

        //Print data and then test the next baudrate
        printf("% 10llu % 10llu % 10llu % 10lld",
               delay_min / time_div,
               delay_avg / time_div,
               delay_max / time_div,
               (((int64_t)delay_avg) - ((int64_t)minimum_delay_ns)) / ((int64_t)time_div)
               );
        if(has_timed_out){
            printf("*T \r\n");
        }
        else if(has_errors){
            printf("*E \r\n");
        }
        else{
            printf("\r\n");
        }
        result_map[baudrate] = std::move(sample_array);
    }

    //Cleanup
    timer.invalidate();

    for (const auto& [rate, samples] : result_map) {
        fout << rate;
        for (const auto sample : samples) fout << " " << sample;
        fout << std::endl;
    }

    QCoreApplication::exit(0);
};

// The program main creates a thread and launches Qt core
// and will wait until someone calls QCoreApplication::exit()
int main(int argc, char *argv[])
{
    int rv;
    QCoreApplication a(argc, argv);
    a.setApplicationName("serial-latency");
    std::shared_ptr<QThread> main_qthread (
        QThread::create(main_thread, argc, argv)
    );
    main_qthread->start();
    rv = a.exec();
    main_qthread->wait();
    return rv;
}
