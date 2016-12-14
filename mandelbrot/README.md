# Mandelbrot

This is an image-parallel Mandelbrot renderer written with Charm++. Each chare is responsible
for a tile in the image, so the rendering can be distributed image-parallel across the processes.

## Arguments

- `--tile W H` specify the width and height of each tile.
- `--img W H` specify the width and height of the image in tiles.
- `--samples N` specify the number of subsamples to take for each pixel

## TODO

I'm not sure how Charm++ handles multithreading on a node, the docs mention this `CkLoop` module
which schedules threads to do work within a chare while sharing resources with the higher level
Charm++ chare process scheduler, however I think the current code which just makes a
chare array and calls render does this on a process-parallel level.

## Example

Here's the result from the default tile and image size parameters with 16 samples per pixel.

![mandelbrot](http://i.imgur.com/Qoz7FBe.png)

