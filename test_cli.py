import subprocess
import time
import sys
import threading

def read_output(process):
    try:
        for line in iter(process.stdout.readline, ''):
            print(line, end='')
    except:
        pass

def main():
    print("Starting BitScrape CLI test...")
    process = subprocess.Popen(
        ['./build/bin/bitscrape_cli'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=0,
        cwd='/Users/jaredterrance/Projects/bitscrape'
    )

    # Start a thread to read output
    t = threading.Thread(target=read_output, args=(process,))
    t.daemon = True
    t.start()
    
    # Wait for startup
    time.sleep(2)
    
    print("\nSending 'start' command...")
    process.stdin.write("start\n")
    process.stdin.flush()
    
    # Run for 60 seconds
    print("\nRunning for 60 seconds...")
    time.sleep(60)
    
    print("\nSending 'stop' command...")
    try:
        process.stdin.write("stop\n")
        process.stdin.flush()
        time.sleep(2)
        process.stdin.write("exit\n")
        process.stdin.flush()
    except:
        pass
        
    process.terminate()
    print("\nTest finished.")

if __name__ == "__main__":
    main()
