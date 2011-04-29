#ifndef MINMAX_H
#define MINMAX_H

#ifndef min
    #define min( _lhs, _rhs ) ( ( _lhs > _rhs )? ( _rhs ) : ( _lhs ) )
#endif

#ifndef max
    #define max( _lhs, _rhs ) ( ( _lhs < _rhs )? ( _rhs ) : ( _lhs ) )
#endif

#endif
