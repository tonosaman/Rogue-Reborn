/*
Rogue Reborn GUI "Image" source from src/image.cpp

Copyright 2010 Justin Bruce Van Horne.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <hat/gui/image.hpp>
#include <hat/gui/ui_local.h>

namespace hat {
   
namespace {
/*
This is a list of accessors made available through the Image class.
Any additional elements that are added to the class should be added here.
They will be looped over and added to the accessor list when a template
is constructed.
*/
JS_mapping accessors[] = {
    JS_MAP(Image, src),
    JS_MAP(Image, angle),
    { NULL, NULL, NULL } // Signals the end of the accessor list
};

/*
This is a list of functions made availabe through the Image class.
Any additional functions that are addeed tot he class should be added here.
They will be looped over and added to the function list when a template
is constructued.
*/
JS_fun_mapping funs[] = {
    { NULL, NULL, NULL } // Signlas the end of the function list
};
}

/*
*/
JS_SETTER_CLASS(Image, src)
{
    Image* i = diamond_unwrap(Image, info.Holder());
    i->image_attrs.src = JS_STR_TO_STL(value->ToString());
    i->image_attrs.src_handle = -1;
    i->element_attrs.modified = true;
}

/*
*/
JS_GETTER_CLASS(Image, src)
{
    Image* i = diamond_unwrap(Image, info.Holder());
    return v8::String::New(i->image_attrs.src.c_str());
}

/*
*/
JS_SETTER_CLASS(Image, angle)
{
    Image* i = diamond_unwrap(Image, info.Holder());
    double integral;
    double fraction = modf(value->NumberValue(), &integral);
    i->image_attrs.angle = integral;
    if (i->image_attrs.angle != 360) {
        i->image_attrs.angle += fraction;
    }
    i->element_attrs.modified = true;
}

/*
*/
JS_GETTER_CLASS(Image, angle)
{
    Image* i = diamond_unwrap(Image, info.Holder());
    return v8::Number::New(i->image_attrs.angle);
}

/*
*/
Image::Image(const Element_attributes& element_attributes,
    const Image_attributes& image_attributes)
{
    name = "SimpleImage";
    element_attrs = element_attributes;
    image_attrs = image_attributes;
}

/*
*/
void Image::think(int ms)
{
    if (!element_attrs.active) {
        return;
    }

    // We think first
    think_fun(ms);

    // Whenever the image has been modified or we get a new screen
    // modification, then we need to re-evaluate the image
    if (image_attrs.src_handle == -1 || global::screen_attrs.modified) {
        Com_Printf("Image::think(%d) for %s - Source handle needs updating to %s\n", 
            ms, name, image_attrs.src.c_str());
        image_attrs.src_handle = trap_R_RegisterShaderNoMip(image_attrs.src.c_str());
        if (!image_attrs.src_handle) {
            Com_Warning("Image::think(%d) for %s - The image could not be loaded.\n",
                ms, name, image_attrs.src.c_str());
        }
    }

    if (!image_attrs.src_handle) {
        return;
    }
    
    // Draw the image
    if (element_attrs.modified || global::screen_attrs.modified) {
        Screen_attributes& screen = global::screen_attrs;
        image_attrs.scaled_x = element_attrs.x * screen.scale_x; 
        image_attrs.scaled_y = element_attrs.y * screen.scale_y; 
        image_attrs.scaled_w = element_attrs.width * screen.scale_x;
        image_attrs.scaled_h = element_attrs.height * screen.scale_y;
    }

    if (image_attrs.angle != 0.0f) {
        // Draw the actual image, just rotated
        trap_R_DrawRotatedPic(image_attrs.scaled_x,
            image_attrs.scaled_y,
            image_attrs.scaled_w,
            image_attrs.scaled_h,
            image_attrs.s1, image_attrs.t1, image_attrs.s2, image_attrs.t2,
            image_attrs.src_handle,
            image_attrs.angle);
    } else {
        // Draw the actual image
        trap_R_DrawStretchPic(image_attrs.scaled_x,
            image_attrs.scaled_y,
            image_attrs.scaled_w,
            image_attrs.scaled_h,
            image_attrs.s1, image_attrs.t1, image_attrs.s2, image_attrs.t2,
            image_attrs.src_handle);
    }

    element_attrs.modified = false;
}


/*
*/
bool Image::build_attributes(const v8::Arguments& args, Element_attributes* ea, Image_attributes* ia)
{
    // There are two conditions that we care about. The first is that we
    // get an argument pattern like:
    //      Image("src", x-pos, y-pos, width, height);
    // and the other is:
    //      Image({ dict });

    if (args.Length() >= 5) {
        v8::Local<v8::Value> src    = args[0];
        v8::Local<v8::Value> x      = args[1];
        v8::Local<v8::Value> y      = args[2];
        v8::Local<v8::Value> width  = args[3];
        v8::Local<v8::Value> height = args[4];
        v8::Local<v8::Value> angle  = args[5];
        JS_BA_STR_REQUIRED(ia->src, src, "Expected a string for str");
        JS_BA_INT_REQUIRED(ea->x, x, "Expected an integer for x.");
        JS_BA_INT_REQUIRED(ea->y, y, "Expected an integer for y.");
        JS_BA_INT_REQUIRED(ea->width, width, "Expected an integer for width.");
        JS_BA_INT_REQUIRED(ea->height, height, "Expected an integer for height`.");
        JS_BA_FLOAT(ia->angle, angle, "Expected a decimal for angle");
    } else if (args.Length() == 1 && args[0]->IsObject()) {
        v8::Local<v8::Object> arg_obj = args[0]->ToObject();
        v8::Local<v8::Value> src    = arg_obj->Get(v8::String::New("src"));
        v8::Local<v8::Value> angle  = arg_obj->Get(v8::String::New("angle"));
        JS_BA_STR_REQUIRED(ia->src, src, "Expected a string for str");
        JS_BA_FLOAT(ia->angle, angle, "Expected a decimal for angle");
    }

    return true;
}

/*
*/
void Image::wrap_extension_list(Extension_list* list)
{
    list->push_back(std::make_pair(accessors, funs));
}

/*
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/

v8::Handle<v8::Value> Image::create(const v8::Arguments& args)
{
    static v8::Persistent<v8::FunctionTemplate> image_tmpl;
    static Extension_list extension_list;

    Com_Printf("Image::Create() - gui.SimpleImage() is being created\n");

    // We only need to build our extension list once
    if (!extension_list.size()) extension_list.push_back(std::make_pair(accessors, funs));

    // Create a new instance of the image, build the attributes,
    // and make sure there are no problems during the build.
    Element_attributes element_attributes;
    Image_attributes image_attributes;

    // Try to build the arguments. If an exceptions is caught, then we
    // need to rethrow it so the GUI can catch it.
    if (!Element::build_attributes(args, &element_attributes) ||
        !Image::build_attributes(args, &element_attributes, &image_attributes))
    {
        return v8::Undefined();
    }

    Image* image = new Image(element_attributes, image_attributes);

    // Now wrap the rest of the image
    Element::wrap_tmpl(&image_tmpl, image, extension_list);
    Com_Printf("Image::Create() - gui.SimpleImage() has been initialized and created.\n");

    return image->self;
}

/*
*/
Eventful_image::Eventful_image(const Element_attributes& element_attributes,
    const Image_attributes& image_attributes)
    : Image(element_attributes, image_attributes)
{
    name = "Image";
}

/*
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/

v8::Handle<v8::Value> Eventful_image::create(const v8::Arguments& args)
{
    static v8::Persistent<v8::FunctionTemplate> image_tmpl;
    static Extension_list extension_list;

    Com_Printf("Eventful_image::Create() - gui.Image() is being created\n");

    // We only need to build our extension list once
    if (!extension_list.size()) {
        extension_list.push_back(std::make_pair(accessors, funs));
        Eventful::wrap_extension_list(&extension_list);
    }

    // Create a new instance of the image, build the attributes,
    // and make sure there are no problems during the build.
    Element_attributes element_attributes;
    Image_attributes image_attributes;

    // Try to build the arguments. If an exceptions is caught, then we
    // need to rethrow it so the GUI can catch it.
    if (!Element::build_attributes(args, &element_attributes) ||
        !Image::build_attributes(args, &element_attributes, &image_attributes))
    {
        return v8::Undefined();
    }

    // Now wrap the rest of the image
    Eventful_image* image = new Eventful_image(element_attributes, image_attributes);
    Com_Printf("Eventful_image::Create() - gui.Image() has been initialized and created.\n");

    return Element::wrap_tmpl(&image_tmpl, image, extension_list);
}

}
