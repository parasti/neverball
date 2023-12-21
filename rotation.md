Neverball camera rotation

In Neverball, you can control the camera rotation with the left or right mouse buttons:
holding down the button rotates the camera in that direction (or opposite that direction,
depending on how you look at it) around the ball.

The baseline speed of rotation is 90 degrees per second. The delta angle for a given frame
is calculated in game_update_view(). That baseline is multiplied by two values: 1) the input
value coming from the input device, and 2) the rotate_fast and rotate_slow speed multiplier
values configured in the neverballrc config file.

Let R be the input value and K be the speed multiplier. Here's the rotation speed formula:

    90 * R * K

With mouse and keyboard, the input value R is simply 1.0 for pressed and 0.0 otherwise.

For a joystick, the input value R is the actual normalized (-1.0 to +1.0) axis tilt. So you
can have much more control over the speed of rotation by using a joystick, and you actually
only reach mouse/keyboard rotation speed by moving the stick to the maximum position.

rotate_slow is applied by default. You can hold down a key or a button to switch to the
rotate_fast value temporarily.

rotate_slow is by default 150 which is divided by 100 to obtain a 1.5 speed multiplier K.

rotate_fast is by default 300, which gives a 3.0 speed multiplier K.

--

With the web version and the touchscreen version that is embedded within the web version,
I am being very intentional about the gameplay controls. There are at least two pet peeves
of mine that I can identify that I wanted to avoid with Neverball:

1) First of all, accelerometer-based controls are a gimmick. They suck really hard, and
here's why: you are literally required to tilt the game away from your face to play it,
and you can't play the game from whatever position you are in (without extra hoops) because
of gravity.

2) On-screen button overlays have their own problems: they cover the game view, are fixed
to a particular location, and provide zero tactile feedback. There are some controls that may need
to be on the screen, but I believe that the fundamental game loop should be enjoyable without
constantly having to precision-target a location on the screen with your thumb.

--

With that out of the way, I have settled on a simple multi-touch control scheme. The
first touch point controls the tilt, the second touch point controls the camera rotation.
This works across the entire screen/canvas, except on the pause and camera toggle overlay
buttons that are intentionally placed out of the way (of thumbs) when applying a conventional grip.

Basically, the camera rotation is controlled by swiping with the other thumb while the first
thumb is touching the screen and controlling the tilt.

The touch motion is filtered for smootheness, and accumulated, giving the impression that you
are controlling an invisible joystick - the input value is determined from how far your thumb
has moved from the original touch point. It is also clamped at 3.0 (actually, it should be
clamped at rotate_fast / 100), in order to access the speeds of rotate_fast.

There is a unique problem with touchscreens - the human thumb. I might say, "moving across 1/16 of
the screen gives the input value of 1.0", but covering 1/16 of a screen with your thumb is easy
on a phone but could be challenging on a tablet in landscape mode. In reality, I probably want to say
something closer to "swiping 0.5 cm with your thumb gives the input value of 1.0". This requires
knowledge of the DPI of the screen to convert touch coordinates into real-world distances.