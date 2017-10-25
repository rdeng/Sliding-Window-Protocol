#include "receiver.h"

void init_receiver(Receiver * receiver,
                   int id)
{
    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->NFE = 0;
    int i;
    for(i = 0; i < 8; i++)
    {
        //receiver->recvQ[i].msg = malloc(sizeof(Frame));
        receiver->recvQ[i].received = false;
    }
}


void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair

    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
//printf("incoming_msgs: %d\n", incoming_msgs_length);

        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);

        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        
        //check whether frame is corrupted
        int corrupted = is_corrupted(raw_char_buf, 64);
//printf("corrupted: %d\n", corrupted);
        if(corrupted == 0 && inframe->recvId == receiver->recv_id)
        {
            struct recvQ_slot *slot;
            slot = &receiver->recvQ[inframe->seqNum%8];
            if(!swpInWindow(inframe->seqNum, receiver->NFE, receiver->NFE+8-1))
                return;
            //slot->msg = inframe;
            slot->msg = malloc(sizeof(Frame));
            slot->msg->sendId = inframe->sendId;
            slot->msg->recvId = inframe->recvId;
            slot->msg->seqNum = inframe->seqNum;
            slot->msg->ackNum = inframe->ackNum;
            slot->msg->flag = inframe->flag;
            memcpy(slot->msg->data, inframe->data, FRAME_PAYLOAD_SIZE);
            slot->msg->crc = inframe->crc;
            fprintf(stderr, "slot index: %d\n", inframe->seqNum%8);
            fprintf(stderr, "slot content: %s\n", inframe->data);
            slot->received = true;
            fprintf(stderr, "seqNum is: %d, NFE is: %d\n", inframe->seqNum, receiver->NFE);
            if(inframe->seqNum == receiver->NFE)
            {
                while(slot->received)
                {
                    printf("<RECV_%d>:[%s]\n", receiver->recv_id, slot->msg->data);
                    slot->received = false;
                    ++receiver->NFE;
                    fprintf(stderr, "new NFE: %d\n", receiver->NFE);
                    slot = &receiver->recvQ[receiver->NFE%8];
                }
            }
            inframe->flag = 1;
            if(receiver->NFE == 0)
                inframe->ackNum = 0;
            else
                inframe->ackNum = receiver->NFE-1;
            fprintf(stderr, "ackNum is : %d\n", inframe->ackNum);
            char* char_buf = convert_frame_to_char(inframe);
            append_crc(char_buf, 64);
            ll_append_node(outgoing_frames_head_ptr, char_buf);
        }
 
        //Free raw_char_buf
        free(raw_char_buf);
        free(inframe);
        free(ll_inmsg_node);
    }
}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);

    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
