/*========== my_main.c ==========

  This is the only file you need to modify in order
  to get a working mdl project (for now).

  my_main.c will serve as the interpreter for mdl.
  When an mdl script goes through a lexer and parser,
  the resulting operations will be in the array op[].

  Your job is to go through each entry in op and perform
  the required action from the list below:

  push: push a new origin matrix onto the origin stack
  pop: remove the top matrix on the origin stack

  move/scale/rotate: create a transformation matrix
                     based on the provided values, then
                     multiply the current top of the
                     origins stack by it.

  box/sphere/torus: create a solid object based on the
                    provided values. Store that in a
                    temporary matrix, multiply it by the
                    current top of the origins stack, then
                    call draw_polygons.

  line: create a line based on the provided values. Stores
        that in a temporary matrix, multiply it by the
        current top of the origins stack, then call draw_lines.

  save: call save_extension with the provided filename

  display: view the image live
  =========================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "parser.h"
#include "symtab.h"
#include "y.tab.h"

#include "matrix.h"
#include "ml6.h"
#include "display.h"
#include "draw.h"
#include "stack.h"
#include "gmath.h"


/*======== void first_pass() ==========
  Inputs:
  Returns:

  Checks the op array for any animation commands
  (frames, basename, vary)

  Should set num_frames and basename if the frames
  or basename commands are present

  If vary is found, but frames is not, the entire
  program should exit.

  If frames is found, but basename is not, set name
  to some default value, and print out a message
  with the name being used.
  ====================*/
size_t first_pass(color *ambient) {
  //in order to use name and num_frames throughout
  //they must be extern variables
  char varied; // acts as a boolean that tells whether vary has been called
  extern int num_frames;
  size_t num_lights;
  extern char name[128];
  int i;

  varied = '\0';
  num_frames = 1;
  num_lights = 0;
  strcpy(name, "image");

  for (i=0;i<lastop;i++) {
    switch (op[i].opcode)
      {
      case FRAMES:
	// printf("Num frames: %4.0f\n", op[i].op.frames.num_frames);
	num_frames = op[i].op.frames.num_frames;
	// printf("Num frames: %d\n", num_frames);
	break;
      case BASENAME:
	strncpy(name, op[i].op.basename.p->name, sizeof(name));
	name[sizeof(name) * sizeof(char) - 1] = '\0';
	// printf("Basename: \"%s\"\n", name);
	// print_symtab();
	break;
      case VARY:
	/*
	printf("Vary: %4.0f %4.0f, %4.0f %4.0f\n",
	       op[i].op.vary.start_frame,
	       op[i].op.vary.end_frame,
	       op[i].op.vary.start_val,
	       op[i].op.vary.end_val);
	*/
	varied = 't'; // set to true
	// printf("varied: %c\n", varied);
	break;
      case AMBIENT:
	    
	printf("Ambient: %6.2f %6.2f %6.2f\n",
	       op[i].op.ambient.c[0],
	       op[i].op.ambient.c[1],
	       op[i].op.ambient.c[2]);
	printf("op[i].op.ambient.c[3]: %lf\n", op[i].op.ambient.c[3]);
	    
	ambient->red = op[i].op.ambient.c[0];
	ambient->green = op[i].op.ambient.c[1];
	ambient->blue = op[i].op.ambient.c[2];
	// printf("ambient->red: %d\n", ambient->red);
	// printf("ambient->green: %d\n", ambient->green);
	// printf("ambient->blue: %d\n", ambient->blue);
	break;
      case LIGHT:
	/*
	printf("Light: %s colors: %lf %lf %lf at: %6.2f %6.2f %6.2f\n",
	       op[i].op.light.p->name,
	       op[i].op.light.c[0], op[i].op.light.c[1],
	       op[i].op.light.c[2],
	       op[i].op.light.l[0], op[i].op.light.l[1],
	       op[i].op.light.l[2]);
	*/
	num_lights++;
	// printf("num_lights: %lu\n", num_lights);
	break;
      }
  }
  if (varied && num_frames <= 1) {
    printf("Error: You called vary but did not set multiple frames.\n");
    exit(EXIT_FAILURE);
  }
  return num_lights;
}

/*======== struct vary_node ** second_pass() ==========
  Inputs:
  Returns: An array of vary_node linked lists

  In order to set the knobs for animation, we need to keep
  a seaprate value for each knob for each frame. We can do
  this by using an array of linked lists. Each array index
  will correspond to a frame (eg. knobs[0] would be the first
  frame, knobs[2] would be the 3rd frame and so on).

  Each index should contain a linked list of vary_nodes, each
  node contains a knob name, a value, and a pointer to the
  next node.

  Go through the opcode array, and when you find vary, go
  from knobs[0] to knobs[frames-1] and add (or modify) the
  vary_node corresponding to the given knob with the
  appropirate value.
  ====================*/
struct vary_node ** second_pass() {
// void second_pass(struct vary_node ** vary)
// {
  struct vary_node **vary;
  int i, j;
  
  vary = (struct vary_node **)calloc(num_frames, sizeof(struct vary_node *));

  /*
  // initialize all frames to null
  for (i = 0; i < num_frames; i++) {
    vary[i] = NULL;
    // printf("vary[i]: %p\n", vary[i]);
  }
  */

  for (i=0;i<lastop;i++) {
    double jump; // number by which to incremement the knob for each frame
    double value; // value of knob in current frame
    struct vary_node *new;
    
    switch (op[i].opcode)
      {
      case VARY:
	/*
	printf("Vary: %4.0f %4.0f, %4.0f %4.0f\n",
	       op[i].op.vary.start_frame,
	       op[i].op.vary.end_frame,
	       op[i].op.vary.start_val,
	       op[i].op.vary.end_val);
	*/
	if (op[i].op.vary.start_frame < 0
	    || op[i].op.vary.end_frame > num_frames
	    || op[i].op.vary.start_frame > op[i].op.vary.end_frame) {
	  printf("Error: Invalid arguments for vary\n");
	  exit(EXIT_FAILURE);
	}
	jump = (op[i].op.vary.end_val - op[i].op.vary.start_val)
	  / (op[i].op.vary.end_frame - op[i].op.vary.start_frame);

	value = op[i].op.vary.start_val;
	for (j = op[i].op.vary.start_frame; j < op[i].op.vary.end_frame; j++) {
	  new = (struct vary_node *)malloc(sizeof(struct vary_node));
	  new->name = op[i].op.vary.p->name;
	  // printf("new->name: \"%s\"\n", new->name);
	  new->value = value;
	  new->next = vary[j];
	  vary[j] = new;
	  value += jump;
	}
	new = (struct vary_node *)malloc(sizeof(struct vary_node));
	new->name = op[i].op.vary.p->name;
	// printf("new->name: \"%s\"\n", new->name);
	new->value = op[i].op.vary.end_val;
	new->next = vary[j];
	vary[j] = new;
	break;
      case LIGHT:
	printf("Light: %s at: %6.2f %6.2f %6.2f\n",
	       op[i].op.light.p->name,
	       op[i].op.light.c[0], op[i].op.light.c[1],
	       op[i].op.light.c[2]);
	/*
	  light[cur_light][LOCATION][0] = op[i].op.light.c[0];
	  light[cur_light][LOCATION][1] = op[i].op.light.c[1];
	  light[cur_light][LOCATION][2] = op[i].op.light.c[2];
	  light[cur_light][COLOR][RED] = 
	*/
	break;
      }
  }
  // printf("vary[0]->name: \"%s\"\n", vary[0]->name);
  return vary;
}

/*======== void print_knobs() ==========
Inputs:
Returns:

Goes through symtab and display all the knobs and their
currnt values
====================*/
void print_knobs() {
  int i;

  printf( "ID\tNAME\t\tTYPE\t\tVALUE\n" );
  for ( i=0; i < lastsym; i++ ) {

    if ( symtab[i].type == SYM_VALUE ) {
      printf( "%d\t%s\t\t", i, symtab[i].name );

      printf( "SYM_VALUE\t");
      printf( "%6.2f\n", symtab[i].s.value);
    }
  }
}

/*======== void my_main() ==========
  Inputs:
  Returns:

  This is the main engine of the interpreter, it should
  handle most of the commadns in mdl.

  If frames is not present in the source (and therefore
  num_frames is 1, then process_knobs should be called.

  If frames is present, the enitre op array must be
  applied frames time. At the end of each frame iteration
  save the current screen to a file named the
  provided basename plus a numeric string such that the
  files will be listed in order, then clear the screen and
  reset any other data structures that need it.

  Important note: you cannot just name your files in
  regular sequence, like pic0, pic1, pic2, pic3... if that
  is done, then pic1, pic10, pic11... will come before pic2
  and so on. In order to keep things clear, add leading 0s
  to the numeric portion of the name. If you use sprintf,
  you can use "%0xd" for this purpose. It will add at most
  x 0s in front of a number, if needed, so if used correctly,
  and x = 4, you would get numbers like 0001, 0002, 0011,
  0487
  ====================*/
void my_main() {

  int i, j;
  struct matrix *trans; // translation matrix (only holds one translation at a time)

  /*
    temporary edge matrix, can hold edges or polygons but not both at the same
    time
  */
  struct matrix *tmp;
  
  struct stack *systems;
  screen t;
  zbuffer zb;
  color g;
  double step_3d = 20;
  double theta;
  size_t num_lights; // number of point light sources
  size_t cur_light; // which light we are up to for adding to the light array

  //Lighting values here for easy access
  color *ambient;
  double view[3];
  double areflect[3];
  double dreflect[3];
  double sreflect[3];

  // struct vary_node *vary[num_frames];
  struct vary_node **vary;
  struct vary_node *dummy;

  ambient = (color *)malloc(sizeof(color) * 1);
  ambient->red = 50;
  ambient->green = 50;
  ambient->blue = 50;

  view[0] = 0;
  view[1] = 0;
  view[2] = 1;

  areflect[RED] = 0.1;
  areflect[GREEN] = 0.1;
  areflect[BLUE] = 0.1;

  dreflect[RED] = 0.5;
  dreflect[GREEN] = 0.5;
  dreflect[BLUE] = 0.5;

  sreflect[RED] = 0.5;
  sreflect[GREEN] = 0.5;
  sreflect[BLUE] = 0.5;

  systems = new_stack();
  tmp = new_matrix(4, 1000);
  clear_screen( t );
  clear_zbuffer(zb);
  g.red = 0;
  g.green = 0;
  g.blue = 0;

  /*
    number of lights is counted in first_pass and used to allocate appropriate
    amount of memory
  */
  num_lights = first_pass(ambient);
  printf("num_lights: %lu\n", num_lights);
  if (!num_lights){
    num_lights = 1;
  }
  double light[num_lights][2][3];

  /*
    Default point light source (will be overwritten if light command is called
    at least once):
  */
  light[0][LOCATION][0] = 0.5;
  light[0][LOCATION][1] = 0.75;
  light[0][LOCATION][2] = 1;
  light[0][COLOR][RED] = 0;
  light[0][COLOR][GREEN] = 255;
  light[0][COLOR][BLUE] = 255;
  cur_light = 0;
  
  vary = second_pass();
  // printf("vary[0]->name: \"%s\"\n", vary[0]->name);

  for (j = 0; j < num_frames; j++) {

    // Set knob values in symbol table
    char framename[135]; // name for the png image containing this frame

    dummy = vary[j];
    
    while (dummy) {
      // print_symtab();
      // printf("dummy->name: \"%s\"\n", dummy->name);
      // printf("dummy->value: %lf\n", dummy->value);
      set_value(lookup_symbol(dummy->name), dummy->value);
      dummy = dummy->next;
    }
    
    for (i=0;i<lastop;i++)
      {
	// printf("%d: ",i);
	switch (op[i].opcode)
	  {
	  case CONSTANTS:
	    areflect[RED] = op[i].op.constants.p->s.c->r[0];
	    areflect[GREEN] = op[i].op.constants.p->s.c->g[0];  
	    areflect[BLUE] = op[i].op.constants.p->s.c->b[0];  

	    dreflect[RED] = op[i].op.constants.p->s.c->r[1];  
	    dreflect[GREEN] = op[i].op.constants.p->s.c->g[1]; 
	    dreflect[BLUE] = op[i].op.constants.p->s.c->b[1]; 
	    
	    sreflect[RED] = op[i].op.constants.p->s.c->r[2]; 
	    sreflect[GREEN] = op[i].op.constants.p->s.c->g[2]; 
	    sreflect[BLUE] = op[i].op.constants.p->s.c->b[2];
            break;      
	  case SAVE_COORDS:
	    printf("Save Coords: %s\n",op[i].op.save_coordinate_system.p->name);
	    break;
	  case CAMERA:
	    printf("Camera: eye: %6.2f %6.2f %6.2f\taim: %6.2f %6.2f %6.2f\n",
		   op[i].op.camera.eye[0], op[i].op.camera.eye[1],
		   op[i].op.camera.eye[2],
		   op[i].op.camera.aim[0], op[i].op.camera.aim[1],
		   op[i].op.camera.aim[2]);

	    break;
	  case SPHERE:
	  /*
          printf("Sphere: %6.2f %6.2f %6.2f r=%6.2f",
                 op[i].op.sphere.d[0],op[i].op.sphere.d[1],
                 op[i].op.sphere.d[2],
                 op[i].op.sphere.r);
          if (op[i].op.sphere.constants != NULL)
            {
              printf("\tconstants: %s",op[i].op.sphere.constants->name);
            }
          if (op[i].op.sphere.cs != NULL)
            {
              printf("\tcs: %s",op[i].op.sphere.cs->name);
            }
	  */
	    add_sphere(tmp, op[i].op.sphere.d[0], op[i].op.sphere.d[1],
		       op[i].op.sphere.d[2],
		       op[i].op.sphere.r, step_3d);
	    matrix_mult(peek(systems), tmp);
	    draw_polygons(tmp, t, zb,
			  view, light, *ambient, areflect, dreflect, sreflect,
			  num_lights);
	    tmp->lastcol = 0;

	    break;
	  case TORUS:
	  /*
          printf("Torus: %6.2f %6.2f %6.2f r0=%6.2f r1=%6.2f",
                 op[i].op.torus.d[0],op[i].op.torus.d[1],
                 op[i].op.torus.d[2],
                 op[i].op.torus.r0,op[i].op.torus.r1);
          if (op[i].op.torus.constants != NULL)
            {
              printf("\tconstants: %s",op[i].op.torus.constants->name);
            }
          if (op[i].op.torus.cs != NULL)
            {
              printf("\tcs: %s",op[i].op.torus.cs->name);
            }
	  */
	    add_torus(tmp, op[i].op.torus.d[0], op[i].op.torus.d[1],
		      op[i].op.torus.d[2],
		      op[i].op.torus.r0, op[i].op.torus.r1, step_3d);
	    matrix_mult(peek(systems), tmp);
	    draw_polygons(tmp, t, zb,
			  view, light, *ambient, areflect, dreflect, sreflect,
			  num_lights);
	    tmp->lastcol = 0;

	    break;
	  case BOX:
	  /*
          printf("Box: d0: %6.2f %6.2f %6.2f d1: %6.2f %6.2f %6.2f",
                 op[i].op.box.d0[0],op[i].op.box.d0[1],
                 op[i].op.box.d0[2],
                 op[i].op.box.d1[0],op[i].op.box.d1[1],
                 op[i].op.box.d1[2]);
          if (op[i].op.box.constants != NULL)
            {
              printf("\tconstants: %s",op[i].op.box.constants->name);
            }
          if (op[i].op.box.cs != NULL)
            {
              printf("\tcs: %s",op[i].op.box.cs->name);
            }
	  */
	    add_box(tmp,
		    op[i].op.box.d0[0],
		    op[i].op.box.d0[1],
		    op[i].op.box.d0[2],
		    op[i].op.box.d1[0],
		    op[i].op.box.d1[1],
		    op[i].op.box.d1[2]);
	    matrix_mult(peek(systems), tmp);
	    draw_polygons(tmp, t, zb,
			  view, light, *ambient, areflect, dreflect, sreflect,
			  num_lights);
	    tmp->lastcol = 0;

	    break;
	  case LINE:
	  /*
          printf("Line: from: %6.2f %6.2f %6.2f to: %6.2f %6.2f %6.2f",
                 op[i].op.line.p0[0],op[i].op.line.p0[1],
                 op[i].op.line.p0[1],
                 op[i].op.line.p1[0],op[i].op.line.p1[1],
                 op[i].op.line.p1[1]);
          if (op[i].op.line.constants != NULL)
            {
              printf("\n\tConstants: %s",op[i].op.line.constants->name);
            }
          if (op[i].op.line.cs0 != NULL)
            {
              printf("\n\tCS0: %s",op[i].op.line.cs0->name);
            }
          if (op[i].op.line.cs1 != NULL)
            {
              printf("\n\tCS1: %s",op[i].op.line.cs1->name);
            }
	  */
	    add_edge(tmp, op[i].op.line.p0[0], op[i].op.line.p0[1],
		     op[i].op.line.p0[1],
		     op[i].op.line.p1[0], op[i].op.line.p1[1],
		     op[i].op.line.p1[1]);
	    matrix_mult(peek(systems), tmp);
	    draw_lines(tmp, t, zb, g);
	    tmp->lastcol = 0;
	    break;
	  case MESH:
	    printf("Mesh: filename: %s",op[i].op.mesh.name);
	    if (op[i].op.mesh.constants != NULL)
	      {
		printf("\tconstants: %s\n",op[i].op.mesh.constants->name);
	      }
	    break;
	  case SET:
	    printf("Set: %s %6.2f\n",
		   op[i].op.set.p->name,
		   op[i].op.set.p->s.value);
	    break;
	  case MOVE:
	  /*
          printf("Move: %6.2f %6.2f %6.2f",
                 op[i].op.move.d[0],op[i].op.move.d[1],
                 op[i].op.move.d[2]);
          if (op[i].op.move.p != NULL)
            {
              printf("\tknob: %s",op[i].op.move.p->name);
            }
	  */

	    if (op[i].op.move.p) // There is a knob
	      trans = make_translate(op[i].op.move.d[0] * op[i].op.move.p->s.value,
				     op[i].op.move.d[1] * op[i].op.move.p->s.value,
				     op[i].op.move.d[2] * op[i].op.move.p->s.value);
	    else
	      trans = make_translate( op[i].op.move.d[0], op[i].op.move.d[1],
				      op[i].op.move.d[2] );
	    matrix_mult(peek(systems), trans);

	    // Replace current top with new top
	    copy_matrix(trans, peek(systems));
	    free_matrix(trans);
	  /*
	  tmp->lastcol = 0;
	  */
	    break;
	  case SCALE:
	  /*
          printf("Scale: %6.2f %6.2f %6.2f",
                 op[i].op.scale.d[0],op[i].op.scale.d[1],
                 op[i].op.scale.d[2]);
          if (op[i].op.scale.p != NULL)
            {
              printf("\tknob: %s",op[i].op.scale.p->name);
            }
	  */
	    if (op[i].op.scale.p) // There is a knob
	      trans = make_scale(op[i].op.scale.d[0] * op[i].op.scale.p->s.value,
				 op[i].op.scale.d[1] * op[i].op.scale.p->s.value,
				 op[i].op.scale.d[2] * op[i].op.scale.p->s.value);
	    else
	      trans = make_scale(op[i].op.scale.d[0], op[i].op.scale.d[1],
				 op[i].op.scale.d[2]);
	    matrix_mult(peek(systems), trans);
	    copy_matrix(trans, peek(systems));
	    free_matrix(trans);
	    break;
	  case ROTATE:
	  /*
          printf("Rotate: axis: %6.2f degrees: %6.2f",
                 op[i].op.rotate.axis,
                 op[i].op.rotate.degrees);
          if (op[i].op.rotate.p != NULL)
            {
              printf("\tknob: %s",op[i].op.rotate.p->name);
            }
	  */
	    theta = op[i].op.rotate.degrees * (M_PI / 180); // convert to radians
	    if (op[i].op.rotate.p) // There is a knob
	      theta *= op[i].op.rotate.p->s.value;
	    
	    if (op[i].op.rotate.axis == 0) // x-axis
	      trans = make_rotX(theta);
	    else if (op[i].op.rotate.axis == 1) // y-axis
	      trans = make_rotY(theta);
	    else if (op[i].op.rotate.axis == 2) // z-axis
	      trans = make_rotZ(theta);
	    else {
	      printf("Error: Invalid axis argument for rotate; must be x, y, or z\n");
	      return;
	    }
	    matrix_mult(peek(systems), trans);
	    copy_matrix(trans, peek(systems));
	    free_matrix(trans);
	    break;
	  case SAVE_KNOBS:
	    printf("Save knobs: %s\n",op[i].op.save_knobs.p->name);
	    break;
	  case TWEEN:
	    printf("Tween: %4.0f %4.0f, %s %s\n",
		   op[i].op.tween.start_frame,
		   op[i].op.tween.end_frame,
		   op[i].op.tween.knob_list0->name,
		   op[i].op.tween.knob_list1->name);
	    break;
	  case PUSH:
	    push(systems);
	    break;
	  case POP:
	    pop(systems);
	    break;
	  case GENERATE_RAYFILES:
	    printf("Generate Ray Files\n");
	    break;
	  case SAVE:
	    save_extension(t, op[i].op.save.p->name);
	    break;
	  case SHADING:
	    printf("Shading: %s\n",op[i].op.shading.p->name);
	    break;
	  case SETKNOBS:
	    printf("Setknobs: %f\n",op[i].op.setknobs.value);
	    break;
	  case FOCAL:
	    printf("Focal: %f\n",op[i].op.focal.value);
	    break;
	  case DISPLAY:
	    display(t);
	    break;
	  }
      }
    sprintf(framename, "anim/%s%03d.png", name, j);
    // printf("framename: %s\n", framename);
    save_extension(t, framename);
    
    free_stack(systems);
    systems = new_stack();
    clear_screen( t );
    clear_zbuffer(zb);
  }
  free_matrix(tmp);
  free_stack(systems);
  free(ambient);

  // Free all the nodes in the knob table
  for (j = 0; j < num_frames; j++) {
    dummy = vary[j];
    while (dummy) {
      struct vary_node *lead_dummy = dummy->next;
      free(dummy);
      dummy = lead_dummy;
    }
  }
  free(vary);
  
  make_animation(name);
}
