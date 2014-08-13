typewriter
==========

The program reads a text file and generates different MIDI note-on events for different keys. To generate actual sound, it needs to be connected to a drum sampler loaded with the distributed wave files.

Dependencies
------------
* mustudio


Currently, the program cannot handle UTF-8 because that would require conversion to full UCS-4. In the meantime, support for setlocale to sv_SE.ISO-8859-1 is required.
