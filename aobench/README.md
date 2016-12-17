# aobench

This example is based on Syoyo Fujita's [aobench](https://github.com/syoyo/aobench) (also see
the [Google code page](https://code.google.com/archive/p/aobench/)).
This implementation will iteratively take samples to allow the Charm++ load balancer to re-balance
the workload while rendering.

## Arguments

- `--tile W H` specify the width and height of each tile.
- `--img W H` specify the width and height of the image in tiles.
- `--spp N` specify the number of samples to take each pixel.
- `--ao-samples N` specify the number of ambient occlusion samples to take at each hit point.

## Example

The image is the default resolution but with 16 AO samples and 100 samples/pixel.

![AOBench](http://i.imgur.com/n1snl19.png)

