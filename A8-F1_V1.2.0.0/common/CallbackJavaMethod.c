/*
 * CallbackJavaMethod.cpp
 *
 *  Created on: 2013-11-14
 *      Author: jerry
 */
#include <jni.h>
#include <stdio.h>
#include <jni.h>
#include <pthread.h>
#include "common/CallbackJavaMethod.h"
#include "common/debugLog.h"

typedef struct CallbackJavaMethod {
	JavaMethodOps ops;
	JNIEnv* jniEnv;
	JavaVM* javaVM;
	pthread_t jvmThread;
	jobject callBackObject;
	jmethodID SystemCallBack_id;
} CallbackJavaMethod, *pCallbackJavaMethod;
typedef struct THREAD_DATA{
	unsigned int dataLen;
	void * data;
	pCallbackJavaMethod JavaMethodServer;
}THREAD_DATA,*pTHREAD_DATA;
int startThreadUp(pJavaMethodOps ops,void *data ,int len);
static void * SystemCallBack(void *arg);

static JavaMethodOps ops = {
		.up = startThreadUp,
};
pJavaMethodOps CallbackJavaMethodInit( JNIEnv *env,
		jobject obj,const char *callbackName) {
	pCallbackJavaMethod javaMethod =(pCallbackJavaMethod)malloc(sizeof(CallbackJavaMethod));
	if(javaMethod == NULL )
	{
		goto fail0;
	}
	(*env)->GetJavaVM(env, &javaMethod->javaVM);
	javaMethod->jniEnv = env;
	javaMethod->jvmThread = pthread_self();
	jclass cls = (*env)->GetObjectClass(env, obj);
	if (cls != NULL) {
		javaMethod->callBackObject =
				(jobject) (*javaMethod->jniEnv)->NewGlobalRef(env, obj);
		javaMethod->SystemCallBack_id = (*env)->GetMethodID(env, cls,
				callbackName,"([B)V"	);
	} else {
		LOGE("Get class form jobject failed.");
	}
	javaMethod->ops = ops;
	return (pJavaMethodOps)javaMethod;
fail1:
	free(javaMethod);
fail0:
	return NULL;
}
void CallbackJavaMethodExit(pJavaMethodOps *ops) {
	JNIEnv* env = NULL;
	pCallbackJavaMethod JavaMethodServer = (pCallbackJavaMethod)*ops;

	LOGE("Try ~CallbackJavaMethod");
	JNIEnv* jniEnv = JavaMethodServer->jniEnv;
	if (JavaMethodServer->callBackObject != NULL) {
		if (jniEnv != NULL) {
			(*jniEnv)->DeleteGlobalRef(jniEnv, JavaMethodServer->callBackObject);
		}
	}
	free(JavaMethodServer);
	*ops = NULL;
}
 int startThreadUp(pJavaMethodOps ops,void *data ,int len)
{

	 pthread_t  pid;
	 pTHREAD_DATA t_data  = (pTHREAD_DATA)malloc(sizeof(THREAD_DATA));
	 if(t_data == NULL)
		 return -1;
	 LOGE("startThreadUp\n");
	 t_data->data = malloc(len);
	 memcpy(t_data->data,data,len);
	 t_data->dataLen = len;
	 t_data->JavaMethodServer = (pCallbackJavaMethod)ops;
	 //线程内部会释放资源
	 if (pthread_create(&pid, 0, SystemCallBack,
			 (void*)t_data) != 0) {
		 free(t_data);
	 		return -1;
	 }
	 pthread_detach(pid);
	 return 0;
}
static void * SystemCallBack(void *arg) {
	pTHREAD_DATA t_data = arg;
	int battach = 0;
	if(arg == NULL)
		goto exit;

	const  jbyte * Jbyte= t_data->data;
	if (t_data->JavaMethodServer->jvmThread != pthread_self()) {
		if ((*(t_data->JavaMethodServer->javaVM))->AttachCurrentThread(
				t_data->JavaMethodServer->javaVM, &t_data->JavaMethodServer->jniEnv, NULL) < 0) {
			goto freeData;
		}
		battach = 1;
	}
	if (t_data->JavaMethodServer->jniEnv != NULL) {
		jbyteArray jarray = (*t_data->JavaMethodServer->jniEnv)->NewByteArray(
				t_data->JavaMethodServer->jniEnv, t_data->dataLen);
		(*(t_data->JavaMethodServer->jniEnv))->SetByteArrayRegion(t_data->JavaMethodServer->jniEnv,
				jarray, 0, t_data->dataLen, Jbyte);
		if (t_data->JavaMethodServer->SystemCallBack_id != NULL) {
			(*(t_data->JavaMethodServer->jniEnv))->CallVoidMethod(t_data->JavaMethodServer->jniEnv,
					t_data->JavaMethodServer->callBackObject,
					t_data->JavaMethodServer->SystemCallBack_id,jarray);
		} else {
			LOGE("callBackObject is null or SystemCallBack_id is null");
		}
		(*(t_data->JavaMethodServer->jniEnv))->DeleteLocalRef(t_data->JavaMethodServer->jniEnv,
				jarray);
		if (battach == 1) {
			(*(t_data->JavaMethodServer->javaVM))->DetachCurrentThread(
					t_data->JavaMethodServer->javaVM);
		}
	}
freeData:
	if(t_data != NULL ){
		if(t_data->data != NULL){
			free(t_data->data);
			t_data->data = NULL;
		}
		free(t_data);
	}
exit:
	return NULL;
}

