# pt

A data-distributed pathtracer which sends rays around the scene to correctly render
large, distributed geomety.

It will currently render this image when run data-parallel with 8 regions. Each sphere and plane
lives on a different "node" and ray tracing is done by sending rays through the "network" to intersect
the different node's local geometry. Currently this is pretty trivial since the geometry is simple, but
my current work is extending it up to support large meshes distributed over a cluster for rendering.

![Prototype image](http://i.imgur.com/E96hgO8.png)

