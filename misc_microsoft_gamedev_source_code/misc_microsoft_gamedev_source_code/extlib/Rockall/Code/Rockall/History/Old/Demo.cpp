

ACTIONS HELLO_WORLD_CLOSURE::ParentBefore( void )
    {
	//
	//   Some initial work ....
	//
	for ( int Count=0;Count < MaxChildren;Count ++ )
		{ 
		new
			(
			HELLO_WORLD_CLOSURE::NewChild,
			this
			)
		HELLO_WORLD_CLOSURE
			(
			Parameter1,
			Parameter2
			);
		}

	return WaitForChildren( ParentAfter );
	}



ACTIONS HELLO_WORLD_CLOSURE::NewChild( void )
    {
	//
	//   Children run here ...
	//

	return Complete();
	}



ACTIONS HELLO_WORLD_CLOSURE::ParentAfter( void )
    {
	//
	//   Parent continues here after children
	//   have finished ...
	//

	return Complete();
	}
