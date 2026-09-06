import std;
import lexer;
import types;

using namespace std;
using namespace wallhalla_n;

int main( const i32 argc, char **argv )
{
  println( "start engine {}!", "Wallhalla" );
  for ( i32 i = 1; i < argc; ++i )
    tokenize( argv[ i ] );
  return 0;
}
