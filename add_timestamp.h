//
// Created by Donguk Lim on 2018. 2. 20..
//

#ifndef PLDA_ADD_TIMESTAMP_H
#define PLDA_ADD_TIMESTAMP_H

#include <streambuf>
#include <iostream>
#include <cassert>

#include <string>
#include <ctime>

class AddTimeStamp : public std::streambuf
{
public:
    AddTimeStamp( std::basic_ios< char >& out )
            : out_( out )
            , sink_()
            , newline_( true )
    {
        sink_ = out_.rdbuf( this );
        assert( sink_ );
    }
    ~AddTimeStamp()
    {
        out_.rdbuf( sink_ );
    }
protected:
    int_type overflow( int_type m = traits_type::eof() )
    {
        if( traits_type::eq_int_type( m, traits_type::eof() ) )
            return sink_->pubsync() == -1 ? m: traits_type::not_eof(m);
        if( newline_ )
        {   // --   add timestamp here
            std::ostream str( sink_ );
            if( !(str << get_time()) ) // add perhaps a seperator " "
                return traits_type::eof(); // Error
        }
        newline_ = traits_type::to_char_type( m ) == '\n';
        return sink_->sputc( m );
    }
private:
    AddTimeStamp( const AddTimeStamp& );
    AddTimeStamp& operator=( const AddTimeStamp& ); // not copyable
    // --   Members
    std::basic_ios< char >& out_;
    std::streambuf* sink_;
    bool newline_;

    std::string get_time();
};


#endif //PLDA_ADD_TIMESTAMP_H
