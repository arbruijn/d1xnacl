#include "MainThreadRunner.h"
#include <ppapi/cpp/rect.h>
#include <ppapi/cpp/fullscreen.h>
#include <SDL/SDL_video.h>
extern "C" {
extern int sdl_main(int argc, const char *argv[], void *);
}
#include <SDL/SDL.h>
#include <SDL/SDL_nacl.h>

const uint32_t kReturnKeyCode = 13;

class SDLWrap : public pp::Instance {
 public:
  SDLWrap(PP_Instance instance) : pp::Instance(instance),
						  sdl_main_thread_(0),
						  width_(0),
						  height_(0),
						  fullscreen_(this) {
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
    fprintf(stderr, "events requested\n");
  }

  ~SDLWrap() {
    if (sdl_main_thread_) {
      pthread_join(sdl_main_thread_, NULL);
    }
  }

  virtual void DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
    printf("did change view, new %dx%d, old %dx%d\n",
	   position.size().width(), position.size().height(),
	   width_, height_);

    if (position.size().width() == width_ &&
	position.size().height() == height_)
      return;  // Size didn't change, no need to update anything.

    if (sdl_thread_started_ == false) {
      width_ = position.size().width();
      height_ = position.size().height();

      SDL_NACL_SetInstance(pp_instance(), width_, height_);
      // It seems this call to SDL_Init is required. Calling from
      // sdl_main() isn't good enough.
      // Perhaps it must be called from the main thread?
      int lval = SDL_Init(SDL_INIT_AUDIO);
      assert(lval >= 0);
      if (0 == pthread_create(&sdl_main_thread_, NULL, sdl_thread, this)) {
	sdl_thread_started_ = true;
      }
    }
  }

  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    //fprintf(stderr, "HandleInputEvent %d\n", (int)event.GetType());
    if (event.GetType() == PP_INPUTEVENT_TYPE_KEYDOWN) {
      pp::KeyboardInputEvent ke(event);
      if (ke.GetKeyCode() == kReturnKeyCode && (ke.GetModifiers() & PP_INPUTEVENT_MODIFIER_ALTKEY)) {
        fullscreen_.SetFullscreen(!fullscreen_.IsFullscreen());
      }
    }
    SDL_NACL_PushEvent(event);
    return true;
  }

private:
  bool sdl_thread_started_;
  pthread_t sdl_main_thread_;
  int width_;
  int height_;
  pp::Fullscreen fullscreen_;

  static void* sdl_thread(void* param) {
    MainThreadRunner *runner = new MainThreadRunner(reinterpret_cast<SDLWrap *>(param));
    sdl_main(0, NULL, runner);
    return NULL;
  }
};

