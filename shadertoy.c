/* Standalone Shadertoy
 * Copyright (C) 2014 Simon Budig <simon@budig.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <getopt.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

static double mouse_x0 = 0;
static double mouse_y0 = 0;
static double mouse_x = 0;
static double mouse_y = 0;

static GLint prog = 0;
static GLenum tex[4];

void
mouse_press_handler (int button, int state, int x, int y)
{
  if (button != GLUT_LEFT_BUTTON)
    return;

  if (state == GLUT_DOWN)
    {
      mouse_x0 = mouse_x = x;
      mouse_y0 = mouse_y = y;
    }
  else
    {
      mouse_x0 = -1;
      mouse_y0 = -1;
    }
}


void
mouse_move_handler (int x, int y)
{
  mouse_x = x;
  mouse_y = y;
}


void
keyboard_handler (unsigned char key, int x, int y)
{
  switch (key)
    {
      case '\x1b':  /* Escape */
      case 'q':
      case 'Q':
        glutLeaveMainLoop ();
        break;

      case 'f': /* fullscreen */
      case 'F':
        glutFullScreenToggle ();
        break;

      default:
        break;
    }
}


void
redisplay (int value)
{
  glutPostRedisplay ();
  glutTimerFunc (value, redisplay, value);
}


void
display (void)
{
  static int frames, last_time;
  int width, height, ticks;
  GLint uindex;

  glUseProgram (prog);

  width  = glutGet (GLUT_WINDOW_WIDTH);
  height = glutGet (GLUT_WINDOW_HEIGHT);
  ticks  = glutGet (GLUT_ELAPSED_TIME);

  if (frames == 0)
    last_time = ticks;

  frames++;

  if (ticks - last_time >= 5000)
    {
      fprintf (stderr, "FPS: %.2f\n", 1000.0 * frames / (ticks - last_time));
      frames = 0;
    }

  uindex = glGetUniformLocation (prog, "iGlobalTime");
  if (uindex >= 0)
    glUniform1f (uindex, ((float) ticks) / 1000.0);

  uindex = glGetUniformLocation (prog, "time");
  if (uindex >= 0)
    glUniform1f (uindex, ((float) ticks) / 1000.0);

  uindex = glGetUniformLocation (prog, "iResolution");
  if (uindex >= 0)
    glUniform3f (uindex, width, height, 1.0);

  uindex = glGetUniformLocation (prog, "iChannel0");
  if (uindex >= 0)
    {
      glActiveTexture (GL_TEXTURE0 + 0);
      glBindTexture (GL_TEXTURE_2D, tex[0]);
      glUniform1i (uindex, 0);
    }

  uindex = glGetUniformLocation (prog, "iChannel1");
  if (uindex >= 0)
    {
      glActiveTexture (GL_TEXTURE0 + 1);
      glBindTexture (GL_TEXTURE_2D, tex[1]);
      glUniform1i (uindex, 1);
    }

  uindex = glGetUniformLocation (prog, "iMouse");
  if (uindex >= 0)
    glUniform4f (uindex,
                 mouse_x,  height - mouse_y,
                 mouse_x0, mouse_y0 < 0 ? -1 : height - mouse_y0);

  uindex = glGetUniformLocation (prog, "resolution");
  if (uindex >= 0)
    glUniform2f (uindex, width, height);

  uindex = glGetUniformLocation (prog, "led_color");
  if (uindex >= 0)
    glUniform3f (uindex, 0.5, 0.3, 0.8);

  glClear (GL_COLOR_BUFFER_BIT);
  glRectf (-1.0, -1.0, 1.0, 1.0);

  glutSwapBuffers ();
}


int
load_texture (char    *filename,
              GLenum   type,
              GLenum  *tex_id)
{
  GdkPixbuf *pixbuf;
  int width, height;
  uint8_t *data;
  GLfloat *tex_data;
  int rowstride;
  int cpp, bps;
  int x, y, c;

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  data = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  bps = gdk_pixbuf_get_bits_per_sample (pixbuf);
  cpp = gdk_pixbuf_get_n_channels (pixbuf);

  if (bps != 8 && bps != 16)
    {
      fprintf (stderr, "unexpected bits per sample: %d\n", bps);
      return 0;
    }

  if (cpp != 3 && cpp != 4)
    {
      fprintf (stderr, "unexpected n_channels: %d\n", cpp);
      return 0;
    }

  tex_data = malloc (width * height * cpp * sizeof (GLfloat));
  for (y = 0; y < height; y++)
    {
      uint8_t  *cur_row8  = (uint8_t *)  (data + y * rowstride);
      uint16_t *cur_row16 = (uint16_t *) (data + y * rowstride);

      for (x = 0; x < width; x++)
        {
          for (c = 0; c < cpp; c++)
            {
              if (bps == 8)
                tex_data[(y * width + x) * cpp + c] = ((GLfloat) cur_row8[x * cpp + c]) / 255.0;
              else
                tex_data[(y * width + x) * cpp + c] = ((GLfloat) cur_row16[x * cpp + c]) / 65535.0;
            }
        }
    }

  glGenTextures (1, tex_id);
  glBindTexture (type, *tex_id);
  glTexImage2D (type, 0, GL_RGBA,
                width, height,
                0, cpp == 3 ? GL_RGB : GL_RGBA,
                GL_FLOAT,
                tex_data);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap (GL_TEXTURE_2D);

  free (tex_data);
  g_object_unref (pixbuf);

  fprintf (stderr, "texture: %s, %dx%d, %d (%d) --> id %d\n",
           filename, width, height, cpp, bps, *tex_id);

  return 1;
}


GLint
compile_shader (const GLenum  shader_type,
                const GLchar *shader_source)
{
  GLuint shader = glCreateShader (shader_type);
  GLint status = GL_FALSE;
  GLint loglen;
  GLchar *error_message;

  glShaderSource (shader, 1, &shader_source, NULL);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE)
    return shader;

  glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &loglen);
  error_message = calloc (loglen, sizeof (GLchar));
  glGetShaderInfoLog (shader, loglen, NULL, error_message);
  fprintf (stderr, "shader failed to compile:\n   %s\n", error_message);
  free (error_message);

  return -1;
}


GLint
link_program (const GLchar *shader_source)
{
  GLint frag, program;
  GLint status = GL_FALSE;
  GLint loglen;
  GLchar *error_message;

  frag = compile_shader (GL_FRAGMENT_SHADER, shader_source);
  if (frag < 0)
    return -1;

  program = glCreateProgram ();

  glAttachShader (program, frag);
  glLinkProgram (program);
  glDeleteShader (frag);

  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (status == GL_TRUE)
    return program;

  glGetProgramiv (program, GL_INFO_LOG_LENGTH, &loglen);
  error_message = calloc (loglen, sizeof (GLchar));
  glGetProgramInfoLog (program, loglen, NULL, error_message);
  fprintf (stderr, "program failed to link:\n   %s\n", error_message);
  free (error_message);

  return -1;
}

void
init_glew (void)
{
  GLenum status;

  status = glewInit ();

  if (status != GLEW_OK)
    {
      fprintf (stderr, "glewInit error: %s\n", glewGetErrorString (status));
      exit (-1);
    }

  fprintf (stderr,
           "GL_VERSION   : %s\nGL_VENDOR    : %s\nGL_RENDERER  : %s\n"
           "GLEW_VERSION : %s\nGLSL VERSION : %s\n\n",
           glGetString (GL_VERSION), glGetString (GL_VENDOR),
           glGetString (GL_RENDERER), glewGetString (GLEW_VERSION),
           glGetString (GL_SHADING_LANGUAGE_VERSION));

  if (!GLEW_VERSION_2_1)
    {
      fprintf (stderr, "OpenGL 2.1 or better required for GLSL support.");
      exit (-1);
    }
}


char *
load_file (char *filename)
{
  FILE *f;
  int size;
  char *data;

  f = fopen (filename, "rb");
  fseek (f, 0, SEEK_END);
  size = ftell (f);
  fseek (f, 0, SEEK_SET);

  data = malloc (size + 1);
  if (fread (data, size, 1, f) < 1)
    {
      fprintf (stderr, "problem reading file %s\n", filename);
      free (data);
      return NULL;
    }
  fclose(f);

  data[size] = '\0';

  return data;
}


int
main (int   argc,
      char *argv[])
{
  char *frag_code = NULL;
  glutInit (&argc, argv);

  static struct option long_options[] = {
      { "texture",  required_argument, NULL,  't' },
      { "help",     no_argument, NULL,        'h' },
      { 0,          0,                 NULL,  0   }
  };

  glutInitWindowSize (800, 600);
  glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE);
  glutCreateWindow ("Shadertoy");

  init_glew ();

  /* option parsing */
  while (1)
    {
      int c, slot;

      c = getopt_long (argc, argv, ":t:?", long_options, NULL);
      if (c == -1)
        break;

      switch (c)
        {
          case 't':
            if (optarg[0] <  '0' || optarg[0] >  '3' ||
                optarg[1] != ':' || optarg[2] == '\0')
              {
                fprintf (stderr, "Argument for texture file needs a slot from 0 to 3\n");
                exit (1);
              }

            slot = optarg[0] - '0';

            if (!load_texture (optarg + 2, GL_TEXTURE_2D, &tex[slot]))
              {
                fprintf (stderr, "Failed to load texture. Aborting.\n");
                exit (1);
              }
            break;

          case 'h':
          case ':':
          default:
            fprintf (stderr, "Usage:\n  %s [options] <shaderfile>\n", argv[0]);
            fprintf (stderr, "Options:    --help\n");
            fprintf (stderr, "            --texture [0-3]:<textureimage>\n");
            exit (c == ':' ? 1 : 0);
            break;
        }
    }

  if (optind != argc - 1)
    {
      fprintf (stderr, "No shaderfile specified. Aborting.\n");
      exit (-1);
    }

  frag_code = load_file (argv[optind]);
  if (!frag_code)
    {
      fprintf (stderr, "Failed to load Shaderfile. Aborting.\n");
      exit (-1);
    }

  prog = link_program (frag_code);
  if (prog < 0)
    {
      fprintf (stderr, "Failed to link shader program. Aborting\n");
      exit (-1);
    }

  glutDisplayFunc  (display);
  glutMouseFunc    (mouse_press_handler);
  glutMotionFunc   (mouse_move_handler);
  glutKeyboardFunc (keyboard_handler);

  redisplay (1000/60);

  glutMainLoop ();

  return 0;
}

