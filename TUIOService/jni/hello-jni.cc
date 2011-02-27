/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <jni.h>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <android/log.h>

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/HelloJni/HelloJni.java
 */
#define D(a) std::runtime_error(a);
extern "C"
jstring
Java_com_sigmusic_tacchi_tuio_TuioTest_stringFromJNI( JNIEnv* env,
                                                  jobject thiz )
{
    ///__android_log_print(ANDROID_LOG_INFO,"helloneon","Testing!!!");

    std::runtime_error("Testing?");
    std::string s = "Hello from JNI !";
    try
    {
        if (std::getenv("NON_EXISTENT_ENVIRONMENT_VARIABLE") == NULL)
            throw std::runtime_error("Hello from JNI (with full C++ support) !");
    }
    catch (std::exception &ex)
    {
        s = ex.what();
    }
    return env->NewStringUTF(s.c_str());
}
