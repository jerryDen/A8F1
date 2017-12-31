#ifndef CALLBACKJAVAMETHOD_H_
#define CALLBACKJAVAMETHOD_H_




typedef struct JavaMethodOps{
	int (*up)(struct JavaMethodOps *,void * ,int  );
}JavaMethodOps,*pJavaMethodOps;

pJavaMethodOps CallbackJavaMethodInit( JNIEnv *env,
		jobject obj,const char *callbackName) ;
void CallbackJavaMethodExit(pJavaMethodOps *server);


#endif /* CALLBACKJAVAMETHOD_H_ */

