#ifndef _COMPIZ_ANIMATION_H
#define _COMPIZ_ANIMATION_H

#define ANIMATION_ABIVERSION 20081221

typedef enum
{
    WindowEventOpen = 0,
    WindowEventClose,
    WindowEventMinimize,
    WindowEventUnminimize,
    WindowEventShade,
    WindowEventUnshade,
    WindowEventFocus,
    WindowEventNum,
    WindowEventNone
} WindowEvent;

typedef enum
{
    AnimEventOpen = 0,
    AnimEventClose,
    AnimEventMinimize,
    AnimEventShade,
    AnimEventFocus,
    AnimEventNum
} AnimEvent;

typedef struct _AnimEffectProperties
{
    void (*updateWindowAttribFunc) (CompWindow *,
				    WindowPaintAttrib *);
    void (*prePaintWindowFunc) (CompWindow *);
    void (*postPaintWindowFunc) (CompWindow *);
    void (*animStepFunc) (CompWindow *, float time);
    Bool (*initFunc) (CompWindow *);
    void (*initGridFunc) (CompWindow *, int *, int *);
    void (*addCustomGeometryFunc) (CompWindow *, int, Box *,
				   int, CompMatrix *);
    void (*drawCustomGeometryFunc) (CompWindow *);
    Bool (*letOthersDrawGeomsFunc) (CompWindow *);
    void (*updateWinTransformFunc) (CompWindow *,
				    CompTransform *);
    Bool (*prePrepPaintScreenFunc) (CompWindow *, int msSinceLastPaint);
    void (*postPrepPaintScreenFunc) (CompWindow *);
    void (*updateBBFunc) (CompOutput *, CompWindow *, Box *);
    void (*cleanupFunc) (CompWindow *);
    void (*refreshFunc) (CompWindow *, Bool animInitialized);
    Bool (*zoomToIconFunc) (CompWindow *);
    Bool modelAnimIs3D;		// TRUE if anim uses model and 3d coords
    Bool useQTexCoord;		// TRUE if effect needs Q texture coordinates
    void *extraProperties;
} AnimEffectProperties;

typedef struct _AnimEffectInfo
{
    char *name; // name of the animation effect, e.g. "animationpack:Implode"

    // to be set to TRUE for the window event animation list(s) that
    // the new animation (value) should be added to
    // (0: open, 1: close, 2: minimize, 3: shade, 4: focus)
    Bool usedForEvents[AnimEventNum];

    AnimEffectProperties properties;
} AnimEffectInfo;

typedef const AnimEffectInfo * AnimEffect;

typedef struct _ExtensionPluginInfo
{
    unsigned int nEffects;
    AnimEffect *effects;

    // Plugin options to be used in "effect options" strings
    unsigned int nEffectOptions;
    CompOption *effectOptions;

    // Non-window functions
    void (*prePaintOutputFunc) (CompScreen *s, CompOutput *output);
} ExtensionPluginInfo;

typedef struct _xy_pair
{
    float x, y;
} Point, Vector;

typedef struct
{
    float x1, x2, y1, y2;
} Boxf;

typedef struct _xyz_tuple
{
    float x, y, z;
} Point3d, Vector3d;

typedef struct _Object
{
    Point gridPosition;		// position on window in [0,1] range
    Point3d position;		// position on screen

    // Texture x, y coordinates will be offset by given amounts
    // for quads that fall after and before this object in x and y directions.
    // Currently only y offset can be used.
    Point offsetTexCoordForQuadBefore;
    Point offsetTexCoordForQuadAfter;
} Object;

typedef struct _Model
{
    Object *objects;
    int numObjects;
    int gridWidth;
    int gridHeight;

    int winWidth;		// keeps win. size when model was created
    int winHeight;

    Vector scale;
    Point scaleOrigin;

    WindowEvent forWindowEvent;
    float topHeight;
    float bottomHeight;
} Model;

// Window properties common to multiple animation effects
typedef struct _AnimWindowCommon
{
    float animTotalTime;
    float animRemainingTime;
    float timestep;		// to be used in updateWindowAttribFunc

    int animOverrideProgressDir; // 0: default dir, 1: forward, 2: backward

    WindowEvent curWindowEvent;
    AnimEffect curAnimEffect;

    FragmentAttrib curPaintAttrib;

    Region drawRegion;
    Bool useDrawRegion;

    XRectangle icon;

    GLushort storedOpacity;

    CompTransform transform;
    Bool usingTransform;     // whether transform matrix is used for the current effect

    float transformStartProgress;
    float transformProgress;

    Model *model;   // for grid engine
} AnimWindowCommon;

typedef enum
{
    AnimDirectionDown = 0,
    AnimDirectionUp,
    AnimDirectionLeft,
    AnimDirectionRight,
    AnimDirectionRandom,
    AnimDirectionAuto
} AnimDirection;
#define LAST_ANIM_DIRECTION 5

typedef void
(*UpdateBBProc) (CompOutput *output,
		 CompWindow * w,
		 Box *BB);

// Base functions for extension plugins to call
typedef struct _AnimBaseFunctions {
    void (*addExtension) (CompScreen *s,
			  ExtensionPluginInfo *extensionPluginInfo);
    void (*removeExtension) (CompScreen *s,
			     ExtensionPluginInfo *extensionPluginInfo);
    CompOptionValue * (*getPluginOptVal)
	(CompWindow *w, ExtensionPluginInfo *extensionPluginInfo, int optionId);
    Bool (*getMousePointerXY) (CompScreen *s, short *x, short *y);
    UpdateBBProc	updateBBScreen;
    UpdateBBProc	updateBBWindow;
    UpdateBBProc	modelUpdateBB;
    UpdateBBProc	compTransformUpdateBB;
    Bool (*defaultAnimInit) (CompWindow * w);
    void (*defaultAnimStep) (CompWindow * w, float time);
    void (*defaultUpdateWindowTransform) (CompWindow *w,
					  CompTransform *wTransform);
    float (*getProgressAndCenter) (CompWindow *w,
				   Point *center);

    float (*defaultAnimProgress) (CompWindow *w);
    float (*sigmoidAnimProgress) (CompWindow *w);
    float (*decelerateProgressCustom) (float progress,
				       float minx,
				       float maxx);
    float (*decelerateProgress) (float progress);
    AnimDirection (*getActualAnimDirection) (CompWindow * w,
					     AnimDirection dir,
					     Bool openDir);
    void (*expandBoxWithBox) (Box *target, Box *source);
    void (*expandBoxWithPoint) (Box *target, float fx, float fy);
    void (*prepareTransform) (CompScreen *s,
			      CompOutput *output,
			      CompTransform *resultTransform,
			      CompTransform *transform);
    AnimWindowCommon * (*getAnimWindowCommon) (CompWindow *w);
    Bool (*returnTrue) (CompWindow *w);
    void (*postAnimationCleanup) (CompWindow *w);
    void (*fxZoomUpdateWindowAttrib) (CompWindow * w,
				      WindowPaintAttrib * wAttrib);
} AnimBaseFunctions;


#define OPTION_GETTERS(extensionBaseFunctions,				\
		       extensionPluginInfo, firstEffectOption)		\
static inline CompOptionValue *						\
animGetOptVal (CompWindow *w,						\
	       int optionId)						\
{									\
    return (extensionBaseFunctions)->getPluginOptVal			\
    	(w, (extensionPluginInfo), optionId - (firstEffectOption));	\
}						\
						\
inline Bool					\
animGetB (CompWindow *w,			\
	  int optionId)				\
{						\
    return animGetOptVal (w, optionId)->b;	\
}						\
						\
inline int					\
animGetI (CompWindow *w,			\
	  int optionId)				\
{						\
    return animGetOptVal (w, optionId)->i;	\
}						\
						\
inline float					\
animGetF (CompWindow *w,			\
	  int optionId)				\
{						\
    return animGetOptVal (w, optionId)->f;	\
}						\
						\
inline char *					\
animGetS (CompWindow *w,			\
	  int optionId)				\
{						\
    return animGetOptVal (w, optionId)->s;	\
}						\
						\
inline unsigned short *				\
animGetC (CompWindow *w,			\
	  int optionId)				\
{						\
    return animGetOptVal (w, optionId)->c;	\
}

#define OPTION_GETTERS_HDR			\
						\
inline Bool					\
animGetB (CompWindow *w,			\
	  int optionId);			\
						\
inline int					\
animGetI (CompWindow *w,			\
	  int optionId);			\
						\
inline float					\
animGetF (CompWindow *w,			\
	  int optionId);			\
						\
inline char *					\
animGetS (CompWindow *w,			\
	  int optionId);			\
						\
inline unsigned short *				\
animGetC (CompWindow *w,			\
	  int optionId);


#define WIN_X(w) ((w)->attrib.x - (w)->output.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->output.top)
#define WIN_W(w) ((w)->width + (w)->output.left + (w)->output.right)
#define WIN_H(w) ((w)->height + (w)->output.top + (w)->output.bottom)

#define BORDER_X(w) ((w)->attrib.x - (w)->input.left)
#define BORDER_Y(w) ((w)->attrib.y - (w)->input.top)
#define BORDER_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define BORDER_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define RAND_FLOAT() ((float)rand() / RAND_MAX)

#define sigmoid(fx) (1.0f/(1.0f+exp(-5.0f*2*((fx)-0.5))))
#define sigmoid2(fx, s) (1.0f/(1.0f+exp(-(s)*2*((fx)-0.5))))

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))


// ratio of perceived length of animation compared to real duration
// to make it appear to have the same speed with other animation effects

#define SPRINGY_ZOOM_PERCEIVED_T 0.55f
#define NONSPRINGY_ZOOM_PERCEIVED_T 0.6f
#define ZOOM_PERCEIVED_T 0.75f


#endif

