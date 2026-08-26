#pragma once
#include <string>

struct MenuItem
{
    using OnSelected = void(*)();

    MenuItem(const std::string& text, OnSelected onSelected)
        : text(text), onSelected(onSelected)
    {
    }

    std::string text;
    OnSelected onSelected = nullptr;
};

