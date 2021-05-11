#include "Database.hpp"
#include "Binary.hpp"

#include <iostream>

enum class EntityOpCode : unsigned char
{
    None = 0x80,
    Byte = 0x81,
    Char32 = 0x82,
    Char16 = 0x83,
    Char8 = 0x84,
    Float64 = 0x85,
    Float32 = 0x86,
    AtString32 = 0x87,
    AtString16 = 0x88,
    String32 = 0x89,
    String16 = 0x8a,
    False = 0x8b,
    True = 0x8c,
    Int64 = 0x8d,
    Int32 = 0x8e,
    Int16 = 0x8f
};

template<int N, typename InputIterator>
int ReadInt(InputIterator &it, const InputIterator & end)
{
    if(it == end) return -1;
    
    if(N==1)
    {
        return (char)(unsigned char)(*it++);
    }
    if(N==2)
    {
        std::int16_t b0 = (unsigned char)*it++;
        std::int16_t b1 = (unsigned char)*it++;
        return (std::int16_t)((b1<<8) | b0);
    }
    int b0 = *it++;
    int b1 = *it++;
    int b2 = *it++;
    int b3 = *it++;

    return b3<<24 | b2<<16 | b1<<8 | b0;
}

template<typename InputIterator>
Entity ReadEntity(InputIterator &it, const InputIterator &end)
{
    unsigned char a = *it++;
    switch((EntityOpCode)a)
    {
    case EntityOpCode::None:
        return Entity(EntityType::None, 0);
    case EntityOpCode::Byte:
        return Entity(EntityType::None, ReadInt<1>(it, end));
    case EntityOpCode::Char32:
        return Entity(EntityType::Char, ReadInt<4>(it, end));
    case EntityOpCode::Char16:
        return Entity(EntityType::Char, ReadInt<2>(it, end));
    case EntityOpCode::Char8:
        return Entity(EntityType::Char, ReadInt<1>(it, end));
    case EntityOpCode::Float64:
        {
            union
            {
                char ch[8];
                double d;
            };
            Entity result;
            result.type = EntityType::Float;
            for(int i=0; i<8; ++i)
                ch[i] = *it++;
            result.f = d;
            return result;
        }
    case EntityOpCode::Float32:
        {
            Entity result;
            result.type = EntityType::Float;
            result.ch[0] = *it++;
            result.ch[1] = *it++;
            result.ch[2] = *it++;
            result.ch[3] = *it++;
            return result;
        }
    case EntityOpCode::AtString32:
        return Entity(EntityType::AtString, ReadInt<4>(it, end));
    case EntityOpCode::AtString16:
        return Entity(EntityType::AtString, ReadInt<2>(it, end));
    case EntityOpCode::String32:
        return Entity(EntityType::String, ReadInt<4>(it, end));
    case EntityOpCode::String16:
        return Entity(EntityType::String, ReadInt<2>(it, end));
    case EntityOpCode::False:
        return Entity(EntityType::Boolean, 0);
    case EntityOpCode::True:
        return Entity(EntityType::Boolean, 1);
    case EntityOpCode::Int64:
        return Entity(EntityType::Integer, ReadInt<8>(it, end));
    case EntityOpCode::Int32:
        return Entity(EntityType::Integer, ReadInt<4>(it, end));
    case EntityOpCode::Int16:
        return Entity(EntityType::Integer, ReadInt<2>(it, end ));
    default:
        return Entity(EntityType::Integer, (int)(char)a);
    }
}

template<int N, typename OutputIterator>
void WriteInt(int i, OutputIterator &it)
{
    std::uint8_t b[4] = { (std::uint8_t)i, (std::uint8_t)(i>>8), (std::uint8_t)(i>>16), (std::uint8_t)(i>>24) };
    
    *it++ = b[0];
    if(N>1) *it++ = b[1];
    if(N>2) *it++ = b[2];
    if(N>3) *it++ = b[3];
    return;
    
    *it++ = (unsigned char)(i & 0xff);
    if(N>1) *it++ = (unsigned char)((i>>8) & 0xff);
    if(N>2) *it++ = (unsigned char)((i>>16) & 0xff);
    if(N>3) *it++ = (unsigned char)((i>>24) & 0xff);
}

template<typename OutputIterator>
void WriteEntity(const Entity &e, OutputIterator &it)
{
    switch(e.type)
    {
    case EntityType::None:
        *it++ = (unsigned char)EntityOpCode::None;
        break;
    case EntityType::Integer:
        if(e.i <= 0x7f && e.i>-0x60)
        {
            WriteInt<1>(e.i, it);
        }
        else if(e.i <= 0x7fff && e.i>=-0x8000)
        {
            *it++ = (unsigned char)EntityOpCode::Int16;
            WriteInt<2>(e.i, it);
        }
        else
        {
            *it++ = (unsigned char)EntityOpCode::Int32;
            WriteInt<4>(e.i, it);
        }
        break;
    case EntityType::Boolean:
        *it++ = e.i ? (unsigned char)EntityOpCode::True : (unsigned char)EntityOpCode::False;
        break;
    case EntityType::String:
        if((unsigned)e.i <= 0xffff)
        {
            *it++ = (unsigned char)EntityOpCode::String16;
            WriteInt<2>(e.i, it);
        }
        else
        {
            *it++ = (unsigned char)EntityOpCode::String16;
            WriteInt<4>(e.i, it);
        }
        break;
    case EntityType::AtString:
        if((unsigned)e.i <= 0xffff)
        {
            *it++ = (unsigned char)EntityOpCode::AtString16;
            WriteInt<2>(e.i, it);
        }
        else
        {
            *it++ = (unsigned char)EntityOpCode::AtString16;
            WriteInt<4>(e.i, it);
        }
        break;
    case EntityType::Float:
        *it++ = (unsigned char)EntityOpCode::Float32;
        *it++ = e.ch[0];
        *it++ = e.ch[1];
        *it++ = e.ch[2];
        *it++ = e.ch[3];
        break;
    case EntityType::Byte:
        *it++ = (unsigned char)EntityOpCode::Byte;
        WriteInt<1>(e.i, it);
        break;
    case EntityType::Char:
        if((unsigned)e.i <= 0xff)
        {
            *it++ = (unsigned char)EntityOpCode::Char8;
            WriteInt<1>(e.i, it);
        }
        else if((unsigned)e.i <= 0xffff)
        {
            *it++ = (unsigned char)EntityOpCode::Char16;
            WriteInt<2>(e.i, it);
        }
        else
        {
            *it++ = (unsigned char)EntityOpCode::Char32;
            WriteInt<4>(e.i, it);
        }
        break;
    }
}

void WriteBinaryEntity(const Entity &e, std::ostreambuf_iterator<char> & output)
{
    WriteEntity(e, output);
}

void WriteBinaryEntity(const Entity &e, std::ostream_iterator<char> & output)
{
    WriteEntity(e, output);
}

void WriteBinaryEntity(const Entity &e, char * & output, char *end)
{
    WriteEntity(e, output);
}

Entity ReadBinaryEntity(std::istreambuf_iterator<char> & input)
{
    return ReadEntity(input, std::istreambuf_iterator<char>());
}

Entity ReadBinaryEntity(std::istream_iterator<char> & input)
{
    return ReadEntity(input, std::istream_iterator<char>());
}

Entity ReadBinaryEntity(char * & input, char * end)
{
    return ReadEntity(input, end);
}
