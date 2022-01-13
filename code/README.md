# ZXD2400 ESP


```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example output

Here is an example console output. In this case `format_if_mount_failed` parameter was set to `true` in the source code. SPIFFS was unformatted, so the initial mount has failed. SPIFFS was then formatted, and mounted again.

```
I (324) example: Initializing SPIFFS
W (324) SPIFFS: mount failed, -10025. formatting...
I (19414) example: Partition size: total: 896321, used: 0
I (19414) example: Opening file
I (19504) example: File written
I (19544) example: Renaming file
I (19584) example: Reading file
I (19584) example: Read from file: 'Hello World!'
I (19584) example: SPIFFS unmounted
```

To erase the contents of SPIFFS partition, run `idf.py erase-flash` command. Then upload the example again as described above.
