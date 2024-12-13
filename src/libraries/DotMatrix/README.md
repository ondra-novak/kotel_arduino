# arduino_r4_dot_matrix
Alternative driver dot matrix on Arduino R4 Wifi


## Why not original driver

The library `Arduino_LED_Matrix.h` is not good class. Because:

- very bad programming style. Mostly C-like 
- Updates matrix 10000x per seconds one by one LED, which causes very low light yield from LEDs
- While it supports animations, it fails at simple operations like setting a specific pixel
- It does not support multiple framebuffers and does not support a virtual framebuffer size with a window.
- Does not support rotating the framebuffer according to the desired device orientation
- Does not take advantage of the compiler's ability to perform various calculations at compile time

## How this driver works

The driver uses the way the individual diodes of the matrix are wired and allows multiple diodes to be activated simultaneously within one cycle. This ultimately means that the refresh rate can drop to 500Hz, which is 20 times lower than the original class.
This clearly has a positive effect on the luminosity of the individual LEDs, which is much brighter.A lower refresh rate also means that more CPU power is available for other calculations.Finally, because the refresh period is more than 1ms, the display can be controlled without using an external timer if your application uses a 1ms resolution scheduler (which is common). Then the matrix control can be managed using a scheduler

## What the driver can't do

It doesn't install interrupt timer. If you need such feature, you need to implement it by
your own. It is therefore necessary to ensure that the appropriate refresh function is called at least 500 times per second, and the driver leaves it up to the programmer to arrange this

## Modes of operation

### Orientation
- portrail
- landscape
- reversed_portrait
- reversed_landscape

### Pixel format

- monochrome (1 bit per pixel)
- intensity+blink (2 bit per pixel)

### Intensity + blink pixel format

- two bits per pixel, 24 bytes per frame
- 00 - black
- 10 - low intensity
- 01 - high intensity
- 11 - blink

## Use in code

### FrameBuffer

**FrameBuffer** declaration of frame buffer. It has three arguments

```
DotMatrix::FrameBuffer<width, height, format>
```

Example:

```
using MyFrameBuffer = DotMatrix::FrameBuffer<12,8,Format::monochrome_1bit>

MyFrameBuffer my_frame_buffer = {};
```

### State

The State object holds rendering state and it is updated for every call of the driver. You should declare its variable somewhere (global variable). 

```
DotMatrix::State state = {};
```

### Driver

**Driver** is template, you need to configure own instance

```
DotMatrix::Driver<FrameBuffer, Orientation>
```

Example:

```
using MyDriver = DotMatrix::Driver<MyFrameBuffer, Orientation::landscape>;

constexpr MyDriver driver = {};
```

It is strongly recommended to declare driver as **constexpr**. This ensures that compiler constructs the instance during compilation and final code contains already precalulated maps required to corrently drive diods

### Driving

```
void loop() {
    driver.drive(state, my_frame_buffer);
}
```

### Start address

You can specify start address in the frame buffer. This allows to create pages or scrolling.


```
void loop() {
    driver.drive(state, my_frame_buffer, offset);   //offset is in bytes
    delay(2);   //2ms 500x
}
```

### Scrolling text from right to left

This is espcially done by configuring the driver in the portrait orientation and by rendeding rotated text about 90 degrees. Then you can slide up or down, which results to scroll the text left or right


### Rendering bitmap

```
DotMatrix::Bitmap<width,height> bmp(data);
BitBlt<BltOp::copy, Rotation::rot0>::bitblt(frame_buffer, bmp, x, y);
```

The bitmap can be created by using an **asciiart** directly in source 

```
constexpr DotMatrix::Bitmap<8,6> icons[] = {
        "O O O   "
        " OOO    "
        "  O  O O"
        "  O   O "
        "  O  O O"
        "        ",
        "O O O   "
        " OOO    "
        "  O    O"
        "  O O O "
        "  O  O  "
        "        "
}
```

The rules are: 
- space character: 0
- non-space character: 1

It is also recommended to declare Bitmap as constexpr, which enables compiler to 
parse the Asciiart duing compile time.

### Text

#### Fonts

fonts are series of bitmaps. Currently there are two available fonts covering  ascii characters 32-127

```
DotMatrix::font_5x3;    //fixed 5x3 (6x4 with margin)
DotMatrix::font_6p;     //proporional (6x6, 7x7 with margin)
```

#### Renter string

To render string, you need to create instance of TextRender template

```
using MyTextRender = DotMatrix::TextRender<BltOp::copy, Rotation::rot>;
```

The first argument specifes Blt operation, which can be `copy`, `and_op`, `or_op` and `xor`. The second argument is Rotation which can be `rot0`,`rot90`,`rot180`,`rot270`

You can call following functions in your instance

```
MyTextRender::render_text(frame_buffer,font,x,y,"text");
```

You can render one character 

```
MyTextRender::render_character(frame_buffer, font, x,y, character);
```





