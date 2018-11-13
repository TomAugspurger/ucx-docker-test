
I have ucx working inside docker containers: https://github.com/TomAugspurger/ucx-docker-test

Questions:

1. Which branch in https://github.com/Akshay-Venkatesh/ucx-py to use?
2. Some absolute paths in the build scripts (at least on devel). Can we use paths relative to the `setup.py`?
3. Is there a specific version of CUDA I should be using?

My next steps:

1. Build ucx-py inside the docker setup
2. Get a basic hello world running
3. Start implementing the Comms
