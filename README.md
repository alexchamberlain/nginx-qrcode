nginx-qrencode
============

Copyright &copy; Alex Chamberlain 2012. Released under the MIT licence.

*nginx-qrencode* is a an addon for Nginx to generate and serve QR codes.

Dependencies
------------

*nginx-qrencode* depends upon ```libqrencode``` and ```libgd```. Please install
these first.

Installation
------------

1. Clone this responsitory
    git clone git://github.com/alexchamberlain/nginx-qrcode.git
2. Download the Nginx source, extract and change the current working directory
   to the Nginx tree.
3. Run ```configure``` with ```--add-module=/path/to/source/nginx-qrcode```
