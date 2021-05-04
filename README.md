# Pico-Roids
Wrote this as a fun test of the 1.2" OLED display I bought for Raspberry Pi years ago
I then thought that maybe I could port it to the Raspberry Pi Pico that I bought

I was pleasantly surprised that that frame rate was OK on the Raspberry Pi since the
demo code that I used from Ardui_PI for doing initial work was very slow. I upped the 
baud rate and optimised the writing to the display to reduce the number of calls since
latency might be a factor.

When I ported this to the Pico, I first tried things out using micro python and realised
if I sent just the same commands using i2c.write it actually worked pretty well (I also used
the ssd1306 library for prototyping but thought it would be fun to do things starting from scratch)
I had a few problems with the wiring connections but when I tried a different combination, it "just worked"
I suppose the moral is, just keep trying!

When I prototyped this using C for the Pico, it took a while to get anything drawing on the the display and then when I did it seemed a bit random. I discovered that some of the settings I'd brought over from the original version didn't seem to be appropriate (there's COLUMN OFFSET which was set to 48 for some reason which skews the whole display - setting this back to 0 made a huge difference!)

Once I'd got the pixel and line drawing code in place I just literally copied and pasted the C++ version of my raspberry-pi asteroids in and then set about removing the C++ specifics.

It's not robust, or even very neat code (all those globals make me twitch - I'd never do that in my real job), but I'm very happy with the outcome. 

Jenks, May 2021
