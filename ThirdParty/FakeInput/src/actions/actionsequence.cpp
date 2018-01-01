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

#include "actionsequence.hpp"

#include <iostream>

namespace FakeInput
{
    ActionSequence::ActionSequence()
    {
    }

    ActionSequence::ActionSequence(const ActionSequence& anotherSequence)
    {
        ActionList::const_iterator it;
        for(it = anotherSequence.actions_.begin(); it != anotherSequence.actions_.end(); ++it)
        {
            actions_.push_back((*it)->clone());
        }
    }

    ActionSequence::~ActionSequence()
    {
        ActionList::iterator it;
        for(it = actions_.begin(); it != actions_.end(); ++it)
        {
            delete (*it);
        }
    }

    Action* ActionSequence::clone() const
    {
        return new ActionSequence(*this);
    }

    ActionSequence& ActionSequence::join(ActionSequence& anotherSequence)
    {
        actions_.push_back(anotherSequence.clone());
        return *this;
    }

    ActionSequence& ActionSequence::press(Key key)
    {
        actions_.push_back(new KeyboardPress(key));
        return *this;
    }

    ActionSequence& ActionSequence::press(KeyType type)
    {
        actions_.push_back(new KeyboardPress(type));
        return *this;
    }

    ActionSequence& ActionSequence::press(MouseButton button)
    {
        actions_.push_back(new MousePress(button));
        return *this;
    }

    ActionSequence& ActionSequence::release(Key key)
    {
        actions_.push_back(new KeyboardRelease(key));
        return *this;
    }

    ActionSequence& ActionSequence::release(KeyType type)
    {
        actions_.push_back(new KeyboardRelease(type));
        return *this;
    }

    ActionSequence& ActionSequence::release(MouseButton button)
    {
        actions_.push_back(new MouseRelease(button));
        return *this;
    }

    ActionSequence& ActionSequence::moveMouse(int dx, int dy)
    {
        actions_.push_back(new MouseRelativeMotion(dx, dy));
        return *this;
    }

    ActionSequence& ActionSequence::moveMouseTo(int x, int y)
    {
        actions_.push_back(new MouseAbsoluteMotion(x, y));
        return *this;
    }

    ActionSequence& ActionSequence::wheelUp()
    {
        actions_.push_back(new MouseWheelUp());
        return *this;
    }

    ActionSequence& ActionSequence::wheelDown()
    {
        actions_.push_back(new MouseWheelDown());
        return *this;
    }

    ActionSequence& ActionSequence::runCommand(const std::string& cmd)
    {
        actions_.push_back(new CommandRun(cmd));
        return *this;
    }

    ActionSequence& ActionSequence::wait(unsigned int milisec)
    {
        actions_.push_back(new Wait(milisec));
        return *this;
    }

    void ActionSequence::send() const
    {
        ActionList::const_iterator it;
        for(it = actions_.begin(); it != actions_.end(); ++it)
        {
            (*it)->send();
        }
    }
}
