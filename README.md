# Gian Tricarico and Jeremy Sharapov Period 5
Graphics engine as final project for Mr. DW's computer graphics class (period 5)

 Working stuff
 * Changing ambient light in the mdl (ambient 200 200 200)
 * Changing/adding multiple lights (light light1 255 0 0 0.5 .75 1)
 * Changing the reflection constants for all future objects (constants cons .1 .5 .7 .1 .5 .7 .1 .5 .7)
 * Mesh (mesh :pyramid.obj)

Typing `make` in the terminal will generate an animation with the default script
al.mdl. There are also targets in the makefile that correspond to some of the
other mdl scripts we have included. One may choose to view these animations by
using the following terminal commands, if one so desires:
* `make scrape`
* `make pyramid`
* `make simple`
* `make robot`