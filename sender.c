#include "sender.h"

void init_sender(Sender * sender, int id)
{
    //TODO: You should fill in this function as necessary
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->LAR = 255;
    sender->LFS = 255;
    sender->sendWindowNotFull = 8;
    int i;
    for(i = 0; i < 8; i++)
    {
        //sender->sendQ[i].msg = (struct Frame*)malloc(sizeof(Frame));
        //sender->sendQ[i].timeout = malloc(sizeof(struct timeval));
        sender->sendQ[i].msg = NULL;
        sender->sendQ[i].used = 0;
        sender->sendQ[i].timeout = NULL;
    }
    sender->seqCount = 0; 
}

struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    return NULL;
}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is //for this sender
    //    5) Do sliding window protocol for sender/receiver pair   
    int incoming_acks_length = ll_get_length(sender->input_framelist_head);
    //printf("acks_length: %d\n", incoming_acks_length);
    while (incoming_acks_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inack_node = ll_pop_node(&sender->input_framelist_head);
        incoming_acks_length = ll_get_length(sender->input_framelist_head);

        char * raw_char_buf = (char *) ll_inack_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        
        //check whether frame is corrupted
        int corrupted = is_corrupted(raw_char_buf, 64);
        
        //printf("corrupted: %d\n", corrupted);
        //printf("inframe->sendId: %d\n", inframe->sendId);
        //printf("sender->send_id: %d\n", sender->send_id);
        //printf("inframe->flag: %d\n", inframe->flag);
        //printf("eval: %d\n", (corrupted == 0) && (inframe->sendId == sender->send_id) && (inframe->flag == 1)); 
        if((corrupted == 0) && (inframe->sendId == sender->send_id) && (inframe->flag == 1))
        {
            //fprintf(stderr, "swpInRe: %d\n", swpInWindow(inframe->ackNum, sender->LAR+1, sender->LFS)); 
            //deliverSWP
            if(swpInWindow(inframe->ackNum, sender->LAR+1, sender->LFS))
            {
                do
                {
                    //struct sendQ_slot *slot;
                    ++sender->LAR;
                    struct sendQ_slot* slot = &sender->sendQ[inframe->seqNum%8];
                    //fprintf(stderr, "slot reset: %d\n", inframe->seqNum%8);
                    sender->sendWindowNotFull++;
                    //free(slot->msg);
                    slot->msg = NULL;
                    free(slot->timeout);
                    slot->timeout = NULL;
                    slot->used = 0;
                    //slot->timeout = malloc(sizeof(struct timeval));
                }while(sender->LAR != inframe->ackNum);
            }
        }
 
        //Free raw_char_buf
        free(raw_char_buf);
        free(inframe);
        free(ll_inack_node);
    }

}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    
        
    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        if(sender->sendWindowNotFull == 0)
            break;
        //Pop a node off and update the input_cmd_length
        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);
            

        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = strlen(outgoing_cmd->message);
        char* msg = outgoing_cmd->message;

        while (msg_length > 0)
        {
            //This is probably ONLY one step you want
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            memset(outgoing_frame, 0, sizeof(Frame));
            outgoing_frame->sendId = outgoing_cmd->src_id;
            outgoing_frame->recvId = outgoing_cmd->dst_id;
            outgoing_frame->flag = 0;
            outgoing_frame->seqNum = ++sender->LFS;
            outgoing_frame->ackNum = 0;
            
            fprintf(stderr, "seqNum is : %d\n", outgoing_frame->seqNum);
            
            if(sender->sendWindowNotFull == 0)
            {
                Cmd* newC = (Cmd *)malloc(sizeof(Cmd));
                newC->src_id = outgoing_cmd->src_id;
                newC->dst_id = outgoing_cmd->dst_id;
                newC->message = msg;
                ll_append_node(&(sender->input_cmdlist_head), newC);
                sender->input_cmdlist_head->prev->next = sender->input_cmdlist_head;
                sender->input_cmdlist_head = sender->input_cmdlist_head->prev;
                break;
                //create a new node
                /*LLnode* new_node;
		new_node = (LLnode *) malloc(sizeof(LLnode));
                Cmd* newC = (Cmd *)malloc(sizeof(Cmd));
                newC->src_id = outgoing_cmd->src_id;
                newC->dst_id = outgoing_cmd->dst_id;
                newC->message = msg;
    		new_node->value = newC;

    		//change the new node prev field to null
    		new_node->prev = NULL;
    		//change the new node next field to sender's input cmd list head
    		new_node->next = sender->input_cmdlist_head;
    		//change the sender's input cmd list head's prev to new node
    		sender->input_cmdlist_head->prev = new_node;*/

    		//return;
            }

            if(msg_length >= FRAME_PAYLOAD_SIZE)
            {
                strncpy(outgoing_frame->data, msg, FRAME_PAYLOAD_SIZE);
                //outgoing_frame->flag = 1;
                msg = msg + FRAME_PAYLOAD_SIZE;
            }
            else
            {
                strcpy(outgoing_frame->data, msg);
                msg = msg + msg_length;
            }
            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            /*int j;
            fprintf(stderr, "before: ");
            for(j = 0; j < 64; j++)
            {
                fprintf(stderr, "%02x", outgoing_charbuf[j]);
            }
            fprintf(stderr, "\n");*/
            append_crc(outgoing_charbuf, 64);
            //printf("crc: %#02X\n", outgoing_frame->crc);
            //printf("frame no.: %d, corrupted: %d\n", 
            //      outgoing_frame->seqNum, 
            //      is_corrupted(convert_frame_to_char(outgoing_frame), 64));
            //Convert the message to the outgoing_charbuf
            /*fprintf(stderr, "after: ");
            for(j = 0; j < 64; j++)
            {
                fprintf(stderr, "%02x", outgoing_charbuf[j]);
            }*/
            struct sendQ_slot *slot;           
            sender->sendWindowNotFull--;
            slot = &sender->sendQ[outgoing_frame->seqNum%8];
            slot->timeout = malloc(sizeof(struct timeval));
            struct timeval* to = slot->timeout;
            gettimeofday(to, NULL);
            to->tv_usec+=100000;
            if(to->tv_usec>=1000000)
            {
                to->tv_usec-=1000000;
                to->tv_sec+=1;
            }
            slot->used = 1;
            
            slot->msg = outgoing_frame;
            ll_append_node(outgoing_frames_head_ptr,
                           outgoing_charbuf);
            //free(outgoing_frame);
            //slot->msg = NULL;
        
            msg_length = strlen(msg);
        }

        //At this point, we don't need the outgoing_cmd
        free(outgoing_cmd->message);
        free(outgoing_cmd);
    }   
}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames
    int i;
    for(i = 0; i < 8; i++)
    {
        if(sender->sendQ[i].msg == NULL || sender->sendQ[i].timeout == NULL || 
           sender->sendQ[i].used == 0)
        {
            continue;
        }
        //struct timeval* to = sender->sendQ[i].timeout;
        struct timeval* currTo = malloc(sizeof(struct timeval));
        gettimeofday(currTo, NULL);
        long result = timeval_usecdiff(sender->sendQ[i].timeout, currTo);
        //printf("chaoshi: %ld", result);
        free(currTo);
        if(result > 0)
        {
            //fprintf(stderr, "errormsg");
            Frame* char_buf = sender->sendQ[i].msg;
            struct timeval* to = sender->sendQ[i].timeout;
            gettimeofday(to, NULL);
            to->tv_usec+=100000;
            if(to->tv_usec>=1000000)
            {
                to->tv_usec-=1000000;
                to->tv_sec+=1;
            }
            sender->sendQ[i].timeout = to;
            char* toSend =  convert_frame_to_char(char_buf);
            append_crc(toSend, 64);
         //fprintf(stderr, "resend corruption: %d\n", is_corrupted(toSend, 64));
            ll_append_node(outgoing_frames_head_ptr, toSend);
            fprintf(stderr, "resend: %d\n", char_buf->seqNum);
        }
    }
}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
