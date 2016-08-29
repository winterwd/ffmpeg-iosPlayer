//
//  FXPlayerView.m

//

//  Copyright (c) 2016年 ZouZeLong. All rights reserved.
//


#import "FXPlayerView.h"
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#include "ffmpegDecoder.h"


#pragma mark - shaders
static const char vertexShaderString[] =
"attribute vec4 position;\n"
"attribute vec2 texcoord;\n"
"uniform mat4 modelViewProjectionMatrix;\n"
"varying vec2 v_texcoord;\n"
"void main()\n"
"{\n"
"   gl_Position = modelViewProjectionMatrix * position;\n"
"   v_texcoord = texcoord.xy;\n"
"}\n";

static const char yuvFragmentShaderString[] =
"varying highp vec2 v_texcoord;\n"
"uniform sampler2D s_texture_y;\n"
"uniform sampler2D s_texture_u;\n"
"uniform sampler2D s_texture_v;\n"
"void main()\n"
"{\n"
"highp float y = texture2D(s_texture_y, v_texcoord).r;\n"
"highp float u = texture2D(s_texture_u, v_texcoord).r - 0.5;\n"
"highp float v = texture2D(s_texture_v, v_texcoord).r - 0.5;\n"
"highp float r = y +             1.402 * v;\n"
"highp float g = y - 0.344 * u - 0.714 * v;\n"
"highp float b = y + 1.772 * u;\n"
"gl_FragColor = vec4(r,g,b,1.0);\n"
"}\n";

static GLfloat modelviewProj[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, -1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

static GLint _uniformSamplers[3];
static GLuint _textures[3];

static bool isValid() {
    return (_textures[0] != 0);
}

static void resolveUniforms(GLuint program) {
    _uniformSamplers[0] = glGetUniformLocation(program, "s_texture_y");
    _uniformSamplers[1] = glGetUniformLocation(program, "s_texture_u");
    _uniformSamplers[2] = glGetUniformLocation(program, "s_texture_v");
}

static bool prepareRender() {
    if (_textures[0] == 0)
        return false;
    
    for (int i = 0; i < 3; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, _textures[i]);
        glUniform1i(_uniformSamplers[i], i);
    }
    return true;
}

static bool validateProgram(GLuint prog) {
	GLint status;
	
    glValidateProgram(prog);
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == GL_FALSE) {
		printf("Failed to validate program %d\n", prog);
        return false;
    }
	
	return true;
}

static GLuint compileShader(GLenum type, const char* sources)
{
	GLint status;
	
    GLuint shader = glCreateShader(type);
    if (shader == 0 || shader == GL_INVALID_ENUM) {
        printf("Failed to create shader %d\n", type);
        return 0;
    }
    
    glShaderSource(shader, 1, &sources, NULL);
    glCompileShader(shader);
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glDeleteShader(shader);
		printf("Failed to compile shader:\n");
        return 0;
    }
	return shader;
}

#pragma mark - gl view

enum {
	ATTRIBUTE_VERTEX,
   	ATTRIBUTE_TEXCOORD,
};

static GLuint          _framebuffer;
static GLuint          _renderbuffer;
static GLint           _backingWidth;
static GLint           _backingHeight;
static GLuint          _program;
static GLint           _uniformMatrix;
static GLfloat         _vertices[8];

static EAGLContext     *_context;


static bool loadShaders() {
    
    bool result = false;
    GLuint vertShader = 0, fragShader = 0;
    
	_program = glCreateProgram();
	
    vertShader = compileShader(GL_VERTEX_SHADER, vertexShaderString);
	if (!vertShader)
        goto exit;
    
	fragShader = compileShader(GL_FRAGMENT_SHADER, yuvFragmentShaderString);
    if (!fragShader)
        goto exit;
    
	glAttachShader(_program, vertShader);
	glAttachShader(_program, fragShader);
	glBindAttribLocation(_program, ATTRIBUTE_VERTEX, "position");
    glBindAttribLocation(_program, ATTRIBUTE_TEXCOORD, "texcoord");
	
	glLinkProgram(_program);
    
    GLint status;
    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
		printf("Failed to link program %d \n", _program);
        goto exit;
    }
    
    result = validateProgram(_program);
    
    _uniformMatrix = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    resolveUniforms(_program);
	
exit:
    
    if (vertShader)
        glDeleteShader(vertShader);
    if (fragShader)
        glDeleteShader(fragShader);
    
    if (result) {
        
        printf("OK setup GL programm \n");
        
    } else {
        
        glDeleteProgram(_program);
        _program = 0;
    }
    
    return result;
}

static void updateVertices(bool res, float width, float height) {
    
    const float dH      = (float)_backingHeight / height;
    const float dW      = (float)_backingWidth	  / width;
    const float dd      = res ? MIN(dH, dW) : MAX(dH, dW);
    const float h       = (height * dd / (float)_backingHeight);
    const float w       = (width  * dd / (float)_backingWidth );
    
    _vertices[0] = - w;
    _vertices[1] = - h;
    _vertices[2] =   w;
    _vertices[3] = - h;
    _vertices[4] = - w;
    _vertices[5] =   h;
    _vertices[6] =   w;
    _vertices[7] =   h;
}


static const GLfloat texCoords[8] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
};


//bool isCanVideoPlay = true;
//bool caculateVertices = true;
//static BOOL glRenderFinished = YES;

struct  videoRender{
    bool isCanVideoPlay;
    bool caculateVertices;
    bool glRenderFinished;
};

typedef struct videoRender videoRender;

static videoRender *render = NULL;
static NSConditionLock *s_renderLock = nil;

void* ffmpeg_videooutput_init()
{
    if (s_renderLock==nil) {
        s_renderLock = [[NSConditionLock alloc] init];
    }
    [s_renderLock lock];
    render = NULL;
    render = (videoRender *)malloc(sizeof(videoRender));
    memset(render, 0, sizeof(videoRender));
    render->isCanVideoPlay = true;
    render->caculateVertices = true;
    render->glRenderFinished = true;
    [s_renderLock unlock];
    
    printf("video:  _kplayer_videooutput_init called! \n");
    
    return render;
}

void ffmpeg_videooutput_setarea( void* extend_handle, int x, int y, int width, int height )
{
    printf("video:  _kplayer_videooutput_setarea called! \n");
}



void ffmpeg_videooutput_render(AVFrame *frame)
{
    [s_renderLock lock];
//    printf("video:  _kplayer_videooutput_render called! \n");
    if (render== NULL) {
        [s_renderLock unlock];
        return;
    }
    if(!render->isCanVideoPlay || NULL == frame) {
        [s_renderLock unlock];
        return;
    }
    render->glRenderFinished = NO;
    [EAGLContext setCurrentContext:_context];
    
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glViewport(0, 0, _backingWidth, _backingHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(_program);
    
    const NSUInteger frameWidth = frame->width;
    const NSUInteger frameHeight = frame->height;
    
    if (render->caculateVertices) {
        updateVertices(1, frameWidth, frameHeight);
        render->caculateVertices = false;
    }
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    if (0 == _textures[0])
        glGenTextures(3, _textures);
    
    const UInt8 *pixels[3] = { frame->data[0], frame->data[1], frame->data[2] };
    const NSUInteger widths[3]  = { frameWidth, frameWidth / 2, frameWidth / 2 };
    const NSUInteger heights[3] = { frameHeight, frameHeight / 2, frameHeight / 2 };
    
    for (int i = 0; i < 3; ++i) {
        
        glBindTexture(GL_TEXTURE_2D, _textures[i]);
        
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     widths[i],
                     heights[i],
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     pixels[i]);
        
        if(GL_NO_ERROR != glGetError() ) {
            NSLog(@"OpenGL glTexImage2D error: %d\n", glGetError());
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    if (prepareRender()) {
        
        glUniformMatrix4fv(_uniformMatrix, 1, GL_FALSE, modelviewProj);
        
        glVertexAttribPointer(ATTRIBUTE_VERTEX, 2, GL_FLOAT, 0, 0, _vertices);
        glEnableVertexAttribArray(ATTRIBUTE_VERTEX);
        glVertexAttribPointer(ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, 0, 0, texCoords);
        glEnableVertexAttribArray(ATTRIBUTE_TEXCOORD);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    [_context presentRenderbuffer:GL_RENDERBUFFER];
    render->glRenderFinished = YES;
    [s_renderLock unlock];

}

 /*

int _kplayer_videooutput_uninit( void* extend_handle )
{
    [s_renderLock lock];
    if (render) {
        free(render);
        render = NULL;
    }
    [s_renderLock unlock];
    return 0;
}
 
 */


@implementation FXPlayerView

- (BOOL) getGlRenderFinishedStatue {
    if (render == NULL) {
        return YES;
    }
    return render->glRenderFinished;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        if (render) {
            render->glRenderFinished = YES;
        }
        
        CAEAGLLayer *eaglLayer = (CAEAGLLayer*) self.layer;
        // CALayer 默认是透明的，必须将它设为不透明才能让其可见
        eaglLayer.opaque = YES;
        // 设置描绘属性，在这里设置不维持渲染内容以及颜色格式为 RGBA8
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];
        // 指定 OpenGL 渲染 API 的版本，在这里我们使用 OpenGL ES 2.0
        EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES2;
        _context = [[EAGLContext alloc] initWithAPI:api];
        
        if (!_context ||
            ![EAGLContext setCurrentContext:_context]) {
            
            NSLog(@"failed to setup EAGLContext");
            self = nil;
            return nil;
        }
        
        glGenFramebuffers(1, &_framebuffer);
        glGenRenderbuffers(1, &_renderbuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
        [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];
        
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_backingWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_backingHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderbuffer);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            
            NSLog(@"failed to make complete framebuffer object %x", status);
            self = nil;
            return nil;
        }
        
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError) {
            
            NSLog(@"failed to setup GL %x", glError);
            self = nil;
            return nil;
        }
        
        if (!loadShaders()) {
            
            self = nil;
            return nil;
        }
        
        NSLog(@"OK setup GL");
    }
    return self;
}

+ (Class) layerClass
{
	return [CAEAGLLayer class];
}

- (void)dealloc
{
    if (_textures[0]) {
        glDeleteTextures(3, _textures);
        _textures[0] = 0;
        _textures[1] = 0;
        _textures[2] = 0;
    }
    
    if (_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    
    if (_renderbuffer) {
        glDeleteRenderbuffers(1, &_renderbuffer);
        _renderbuffer = 0;
    }
    
    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
    }
	
	if ([EAGLContext currentContext] == _context) {
		[EAGLContext setCurrentContext:nil];
	}
    
	_context = nil;
    
    [s_renderLock lock];
    if (render) {
        render->caculateVertices = true;
    }
    [s_renderLock unlock];
    
}
@end




 

