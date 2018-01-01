/**
 * This file is part of the FakeInput library (https://github.com/uiii/FakeInput)
 *
 * Copyright (C) 2011 by Richard Jedlicka <uiii.dev@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FI_MOUSEACTION_HPP
#define FI_MOUSEACTION_HPP

#include "action.hpp"
#include "mouse.hpp"

namespace FakeInput
{
    // -------- MOUSE BUTTONS --------- //
    typedef void (*MouseButtonActionCallback)(MouseButton button);

    /** Represents mouse button action. */
    class MouseButtonAction : public Action
    {
    public:
        /** MouseButtonAction constructor.
         *
         * @param callback
         *     Callback which can handle with mouse button.
         * @param button
         *     Mouse button to be handled.
         */
        MouseButtonAction(MouseButtonActionCallback callback, MouseButton button);

        /** Creates new copy of this action.
         *
         * @returns
         *     New copy of this action allocated on heap.
         */
        Action* clone() const;

        /** Performs this action. */
        void send() const;

    private:
        MouseButtonActionCallback callback_;
        MouseButton button_;
    };

    /** Represents mouse button press action. */
    class MousePress : public MouseButtonAction
    {
    public:
        /** MousePress action constructor.
         *
         * @param button
         *     Mouse button to press.
         */
        MousePress(MouseButton button);
    };

    /** Represents mouse button release action. */
    class MouseRelease : public MouseButtonAction
    {
    public:
        /** MouseRelease action constructor.
         *
         * @param button
         *     Mouse button to release.
         */
        MouseRelease(MouseButton button);
    };

    // -------- MOUSE MOTION --------- //
    typedef void (*MouseMotionActionCallback)(int x, int y);

    /** Represents mouse motion action. */
    class MouseMotionAction : public Action
    {
    public:
        /** MouseButtonAction constructor.
         *
         * @param callback
         *     Callback which can move mouse.
         * @param x
         *     How to operate on X axis.
         * @param y
         *     How to operate on Y axis.
         */
        MouseMotionAction(MouseMotionActionCallback callback, int x, int y);

        /** Creates new copy of this action.
         *
         * @returns
         *     New copy of this action allocated on heap.
         */
        Action* clone() const;

        /** Performs this action. */
        void send() const;

    private:
        MouseMotionActionCallback callback_;
        int x_;
        int y_;
    };

    /** Represents mouse relative motion action.
     *
     * Relative means to move mouse some amount of pixels in some direction.
     */
    class MouseRelativeMotion : public MouseMotionAction
    {
    public:
        /** MouseAbsoluteMotion constructor.
         *
         * @param dx
         *     Amount of pixels to move on X axis.
         * @param dy
         *     Amount of pixels to move on Y axis.
         */
        MouseRelativeMotion(int dx, int dy);
    };

    /** Represents mouse absolute motion action.
     *
     * Absolute means to move mouse on the specified position.
     */
    class MouseAbsoluteMotion : public MouseMotionAction
    {
    public:
        /** MouseAbsoluteMotion constructor.
         *
         * @param x
         *     Position on X axis to move on.
         * @param y
         *     Position on Y axis to move on.
         */
        MouseAbsoluteMotion(int x, int y);
    };

    // -------- MOUSE WHEEL --------- //
    typedef void (*MouseWheelActionCallback)();

    class MouseWheelAction : public Action
    {
    public:
        /** MouseWheelAction constructor.
         *
         * @param callback
         *     Callback which can handle with mouse wheel.
         */
        MouseWheelAction(MouseWheelActionCallback callback);

        /** Creates new copy of this action.
         *
         * @returns
         *     New copy of this action allocated on heap.
         */
        Action* clone() const;

        /** Performs this action. */
        void send() const;

    private:
        MouseWheelActionCallback callback_;
    };

    /** Represents mouse wheel up rotation action. */
    class MouseWheelUp : public MouseWheelAction
    {
    public:
        /** MouseWheelUp action constructor. */
        MouseWheelUp();
    };

    /** Represents mouse wheel down rotation action. */
    class MouseWheelDown : public MouseWheelAction
    {
    public:
        /** MouseWheelDown action constructor. */
        MouseWheelDown();
    };
}

#endif
