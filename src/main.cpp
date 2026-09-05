import std;
import lexer;
import types;

using namespace std;
using namespace wallhalla_n;

int main( const i32, char ** )
{
  tokenize();
  println( "hello {}!", "world" );
  return 0;
}
