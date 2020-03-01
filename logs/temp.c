// data is written to head and read from tail!!!

/*
* Simple read write with no handshaking
*/
void block_write2( stream_id_type stream_id, uint32_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	while(1)
	{
		// read tail pointer
		uint64_t temp_tail = stream->buff[TAIL_POINTER];
		
		if( (stream->head + 1 == temp_tail) )
		{
			// can not read
        } 
        else 
        {
        	stream->buff[stream->head] = data;
            stream->head++;  //increment the head
            stream->head == stream->head % stream->buff_size
            // write our new updated position
            stream->buff[HEAD_POINTER] = stream->head;
            break;
        }
	}
}

// data is written to head and read from tail!!!
uint32_t block_read2( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int timeout = 0;
	uint32_t data = 0;
	while(1)
	{
		// read head pointer
		uint64_t temp_head = stream->buff[HEAD_POINTER];

		// head pointer is consumer and he has written something go get it
		if( stream->tail != temp_head )
		{ //see if any data is available
               data = stream->buf[stream->tail];  //grab a byte from the buffer
               stream->tail++;  //incrementthe tail
               stream->tail == stream->tail % stream->buff_size
               // write our new updated position
               stream->buff[TAIL_POINTER] = stream->tail;
               return data;
        } 
		// head and tail are same unable to read anything
		else
		{
			// wait some time
			timeout++;
			if(timeout == 2000)
				break;
		}
	}

	//
	return -1;
}