See <https://docs.google.com/document/d/1ovaW290Zh7sa-D7bZsDYL0_xVdbBMB1nymIhKqlLl_M/edit?disco=AAAAAFVsqfs#>.

How do I compile breakout to run on the Propeller?
--------------------------------------------------

Assuming you have a Propeller board connected to your computer and you know what
port it is on (default port is `/dev/ttyUSB0`, see `BUILD`), this is pretty easy
for the examples.

You can type the following at the operating system prompt to rebuild an
executable Propeller GCC binary:

```
  $ b3 build :breakout
```

To load the executable into the Propeller, type:

```
  $ b3 build :breakout-load
```

Note, this will also rebuild breakout binary, so if you're going to build a
binary and then load it, you can simply use the last command for both.