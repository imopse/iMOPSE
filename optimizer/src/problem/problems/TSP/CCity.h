
#pragma once

class CCity
{
public:
    CCity(int id, float x, float y) {
        m_ID = id;
        m_PosX = x;
        m_PosY = y;
    }

    int m_ID;
    float m_PosX, m_PosY;
};
