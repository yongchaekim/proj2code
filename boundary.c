static bool readerboundchecker(struct r_*new)
{
        int distance = abs(rotation - (new->degree));
        return min(distance, 360 - distance) <= (new->range); // returns whether the rotation is within the range of the reader
}

static bool writerboundchecker(struct w_ *new)
{
        int distance = abs(rotation - (new->degree));
        return min(distance, 360 - distance) <= (new->range); // returns whether the rotation is within the range of the writer
}

static bool validRange(int range)
{
        return 0 < range && range < 180; // check valid range usully checked before calling spinlock_lock
}

static bool validDegree(int degree)
{
        return 0 <= degree && degree < 360; // check valid degree usually checked before calling spinlock_lock
}

static void assignwriterLock(struct w_ *new) {
        list_del(&new->list); // delete the waiting writer list
        list_add(&new->list, &writeracquire_list.list); // put waitingwriters list on to the locked writers list
        new->flag = true; // flags to signal that successful lock is grabbed
}
static void assignreaderLock(struct r_ *new) {
        list_del_init(&new->list); // delete the waiting reader list
        list_add(&new->list, &readeracquire_list.list); // put waitingreaders list on to the locked readers list
        new->flag = true; // flgas to signal that successful lock is grabbed
}

static void findLockAssign(void) {
        //In this function I made 4 list, waitinglist: reader_list, and writer_list. locklist: acquirereader_list, and acquirewriter_list. 
        int writelow = 0;
        int writehigh = 0;
        int readlow = 0;
        int readhigh = 0;
        int curlow = 0;
        int curhigh = 0;
        struct w_ *write;
        struct w_ *write_n;
        struct r_ *read;
        struct r_ *read_n;
        bool writeflag = true;

        //here we have to find the range of the current locked writers so that we are sure that the waitingwriters  range does not overlap with the locked writers
        list_for_each_entry_safe(write, write_n, &writeracquire_list.list, list)
        {
                if(writerboundchecker(write)) // if rotation is in the range of the lockedwriters, no waiting writers or readers can grab a lock so return immediately
                        return;

                writelow = write->range - write->degree; //Get the lock writers low bound 
                writehigh = write->range + write->degree; // Get the lock writers high bound

                if(writelow < 0)   // if writelow < 0 then writelow <= range <= 360;
                {
                        curhigh = 360 + writelow;
                }

                if(writehigh > 360) // if writehigh > 360 then 0 <= range <= writehigh
                {
                        curlow = writehigh - 360;
                }

                writehigh = min(writehigh, curhigh);
                writelow = max(writelow, curlow);
                curlow = 0;
                curhigh = 0;
        }

        //here we have to find the range of the current locked readers so that we are sure that the waitingreaders range does not overlap with the locked readers
        list_for_each_entry_safe(read, read_n, &readeracquire_list.list, list)
        {
                if(readerboundchecker(read)) // if rotation is in the range of the lockedreaders, no waiting writers can grab the lock, only waiting readers
                        writeflag = false;

                readlow = read->range - read->degree;
                readhigh = read->range + read->degree;

                if(readlow < 0)
                {
                        curhigh = 360 + readlow; // similarly computation with the above
                }
                if(readhigh > 360)
                {
                        curlow = readhigh - 360;
                }

                readhigh = min(readhigh, curhigh);
                readlow = max(readlow, curlow);
                curlow = 0;
                curhigh = 0;
        }
        //Now we know that writers cannot be starved to death and we know the readlow, readhigh, writehigh, and writelow. We have to assign first the writer to grab a lock(that means removing the 
        //waitingwriters from the list and putting it on to the locked writers list...

        list_for_each_entry_safe(write, write_n, &writer_list.list, list)
        {
                if(writerboundchecker(write)) // if rotation is within the range of waitingwriters proceed, else get the next iteration
                {
                        if(!writeflag)
                                return;
                        curlow = write->range - write->degree;
                        curhigh = write->range + write->degree;

                        if(curlow < 0)
                        {
                                curlow = 360 - curlow;
                        }
                        if(curhigh > 360)
                        {
                                curhigh = curhigh + 360;
                        }
                        if((writelow < curlow && curhigh > writehigh) && (readlow < curlow && curhigh > readhigh))
                        {
                                assignwriterLock(write); // when it finally grabs the lock, return
                                return;
                        }
                }

        }
        // same fore here but we need to compare only the writer lock range: writehigh and writelow, since many waiting readers can grab many readers lock
        list_for_each_entry_safe(read, read_n, &reader_list.list, list)
        {
                if(readerboundchecker(read))
                {
                        curlow = read->range - read->degree;
                        curhigh = read->range + read->degree;
                        if(curlow < 0)
                        {
                                curlow = 360 - curlow;
                        }
                        if(curhigh > 360)
                        {
                                curhigh = curhigh + 360;
                        }
                        if(writelow < curlow && curhigh > writehigh)
                        {
                                assignreaderLock(read);
                                return;
                        }
                }
        }
}
