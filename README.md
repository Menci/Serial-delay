# [Serial Delay](https://gitlab.com/heavydeck-projects/hardware/Serial-delay) with detailed output

The original repo is the tool used in this article: [Measuring PC serial port latency](https://www.heavydeck.net/blog/measuring-serial-port-latency/).  
It only prints aggregated data. I added the ability to output the raw detailed data, containing all samples.

```bash
serial-delay.exe COM3 ./path/to/detail.txt 600,9600,115200
serial-delay.exe COM3 ./path/to/detail.txt # Default to original behavior (ALL standard baud rates)
```

The format of `detail.txt` is:

```
600 latency1 latency2 latency3 ...
9600 latency1 latency2 latency3 ...
115200 latency1 latency2 latency3 ...
...
[baud rate] [list of latency] [separated with spaces]
```
