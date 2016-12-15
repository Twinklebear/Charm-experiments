# Mandelbrot

This is an image-parallel Mandelbrot renderer written with Charm++. Each chare is responsible
for a tile in the image, so the rendering can be distributed image-parallel across the processes.

## Arguments

- `--tile W H` specify the width and height of each tile.
- `--img W H` specify the width and height of the image in tiles.
- `--samples N` specify the number of subsamples to take for each pixel along each axis. The
	actual number of samples taken will be the square of this value.

## Multithreading Notes

To run with 1 threaded process per node you can also pass the `+ppn M` argument, specifying each node has M
cores. So for example on an 8 core machine running this mandelbrot example:

```
./charmrun ./mandelbrot.out +p 8 +ppn 8
```

Now Charm++ will run 8 processes in total with 8 lightweight thread "processes" per node, so the result
is one process with 8 threads used to compute the Mandelbrot set.

## Example

Here's the result from the default tile and image size parameters with 16 samples per pixel.

![mandelbrot](http://i.imgur.com/Qoz7FBe.png)

