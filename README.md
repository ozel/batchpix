batchpix
========

experimental fork of John Idarraga’s batchpix repo:
http://svnweb.cern.ch/world/wsvn/idarraga/batchpix/PixelmanBatch
based on rev. 4830 with slight modifications to support Medipix MXR/USB
Lite and Timepix1/MX-10.
Also allows endless acquisition if AcqCounts is set to 0 in
configure_runt.txt. This is usefull in conjunction with a mkfifo FIFO file, 
can pipe pixel data directly into mafalda -> realtime processing and display!

