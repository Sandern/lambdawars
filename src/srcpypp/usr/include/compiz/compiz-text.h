/*
 * text.h - adds text image support to beryl.
 * Copyright: (C) 2006 Patrick Niklaus
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _COMPIZ_TEXT_H
#define _COMPIZ_TEXT_H

#define TEXT_ABIVERSION 20090103

/**
 * Flags to be passed into the flags field of CompTextAttrib
 */
#define CompTextFlagStyleBold      (1 << 0) /**< render the text in bold */
#define CompTextFlagStyleItalic    (1 << 1) /**< render the text italic */
#define CompTextFlagEllipsized     (1 << 2) /**< ellipsize the text if the
					         specified maximum size is too
						 small */
#define CompTextFlagWithBackground (1 << 3) /**< render a rounded rectangle as
					          background behind the text */
#define CompTextFlagNoAutoBinding  (1 << 4) /**< do not automatically bind the
					         rendered text pixmap to a
						 texture */

/**
 * Input data structure that specifies how the text is to be rendered
 */
typedef struct _CompTextAttrib {
    char           *family;    /**< font family */
    int            size;       /**< font size in points */
    unsigned short color[4];   /**< font color (RGBA) */

    unsigned int   flags;      /**< rendering flags, see above */

    int            maxWidth;   /**< maximum width of the generated pixmap */
    int            maxHeight;  /**< maximum height of the generated pixmap */

    int            bgHMargin;  /**< horizontal margin in pixels
				    (offset of text into background) */
    int            bgVMargin;  /**< vertical margin */
    unsigned short bgColor[4]; /**< background color (RGBA) */
} CompTextAttrib;

/**
 * Output data structure that represents the rendered text
 */
typedef struct _CompTextData {
    CompTexture  *texture; /**< texture the text pixmap is bound to */
    Pixmap       pixmap;   /**< text pixmap */
    unsigned int width;    /**< pixmap width */
    unsigned int height;   /**< pixmap height */
} CompTextData;

/**
 * Prototype of text-to-pixmap rendering function
 *
 * @param s       screen the text is rendered on
 * @param text    text to be rendered in ASCII or UTF-8 encoding
 * @param attrib  text rendering attributes
 *
 * @return        valid text data on success, NULL on failure
 */
typedef CompTextData *
(*RenderTextProc) (CompScreen           *s,
		   const char           *text,
		   const CompTextAttrib *attrib);

/**
 * Prototype of window title-to-pixmap rendering function
 *
 * @param s                   screen the text is rendered on
 * @param window              XID of the window whose title should be rendered
 * @param withViewportNumber  also render the viewport number behind
 *                            the window title
 * @param attrib              text rendering attributes
 *
 * @return                    valid text data on success, NULL on failure
 */
typedef CompTextData *
(*RenderWindowTitleProc) (CompScreen           *s,
			  Window               window,
			  Bool                 withViewportNumber,
			  const CompTextAttrib *attrib);

/**
 * Prototype of function drawing text data on screen
 *
 * Should only be called for text data which was automatically bound
 * to a texture.
 * NOTE: assumes that x and y are specified in the coordinate system
 *       currently used by OpenGL. For being able to specify screen
 *       coordinates, the caller needs to generate a transformation matrix
 *       using transformToScreenSpace and load it prior to calling this
 *       function.
 *
 * @param s      screen the text is rendered on
 * @param data   text data structure
 * @param x      x position in current OpenGL coordinates
 * @param y      y position in current OpenGL coordinates
 * @param alpha  opacity for the drawn text (0.0 - transparent, 1.0 - opaque)
 */
typedef void (*DrawTextProc) (CompScreen         *s,
			      const CompTextData *data,
			      float              x,
			      float              y,
			      float              alpha);

/**
 * Prototype of text data cleanup function
 *
 * @param s     screen the data was generated for
 * @param data  data structure
 */
typedef void (*FiniTextDataProc) (CompScreen   *s,
				  CompTextData *data);

/**
 * Function pointer set that provides access to the
 * above defined functions.
 */
typedef struct _TextFunc {
    RenderTextProc        renderText;
    RenderWindowTitleProc renderWindowTitle;
    DrawTextProc          drawText;
    FiniTextDataProc      finiTextData;
} TextFunc;

#endif
