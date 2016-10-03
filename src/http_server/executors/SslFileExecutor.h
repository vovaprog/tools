#ifndef SSL_FILE_EXECUTOR_H
#define SSL_FILE_EXECUTOR_H

#include <Executor.h>
#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <FileExecutor.h>

class SslFileExecutor: public FileExecutor
{

protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count) override
    {
        return SSL_write(data.ssl, buf, count);
    }

    ProcessResult process_sendFile(ExecutorData &data) override
    {
        void *p;
        int size;
        if(data.fd1 > 0)
        {
            if(data.buffer.startWrite(p, size))
            {
                int bytesRead = read(data.fd1, p, size);

                if(bytesRead < 0)
                {
                    return ProcessResult::removeExecutor;
                }
                else if(bytesRead == 0)
                {
                    close(data.fd1);
                    data.fd1 = -1;
                }

                data.buffer.endWrite(bytesRead);
            }
        }
        if(data.buffer.startRead(p, size))
        {
            int bytesWritten = SSL_write(data.ssl, p, size);

            if(bytesWritten > 0)
            {
                data.buffer.endRead(bytesWritten);
            }
            else
            {
                log->error("SSL_write failed\n");
                return ProcessResult::removeExecutor;
            }

            data.bytesToSend -= bytesWritten;

            if(data.bytesToSend == 0)
            {
                return ProcessResult::removeExecutor;
            }
        }
        else
        {
            if(data.fd1 > 0)
            {
                log->error("buffer.startRead failed\n");
                return ProcessResult::removeExecutor;
            }
        }

        return ProcessResult::ok;
    }

};



#endif
