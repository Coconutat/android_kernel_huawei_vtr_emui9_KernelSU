#ifndef __ACM_H__
#define __ACM_H_

//Returns 0 if in the white list or a none-zero error code not in the white list.
int acm_search(char *pkgname,struct dentry *dentry,uid_t uid, int file_type);

#endif /* __ACM_H_ */
