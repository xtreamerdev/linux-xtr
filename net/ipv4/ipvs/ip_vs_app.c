/*
 * ip_vs_app.c: Application module support for IPVS
 *
 * Version:     $Id: ip_vs_app.c,v 1.17 2003/03/22 06:31:21 wensong Exp $
 *
 * Authors:     Wensong Zhang <wensong@linuxvirtualserver.org>
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Most code here is taken from ip_masq_app.c in kernel 2.2. The difference
 * is that ip_vs_app module handles the reverse direction (incoming requests
 * and outgoing responses).
 *
 *		IP_MASQ_APP application masquerading module
 *
 * Author:	Juan Jose Ciarlante, <jjciarla@raiz.uncu.edu.ar>
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/init.h>
#include <net/protocol.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <asm/system.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>

#include <net/ip_vs.h>

EXPORT_SYMBOL(register_ip_vs_app);
EXPORT_SYMBOL(unregister_ip_vs_app);
EXPORT_SYMBOL(register_ip_vs_app_inc);

/* ipvs application list head */
static LIST_HEAD(ip_vs_app_list);
static DECLARE_MUTEX(__ip_vs_app_mutex);


/*
 *	Get an ip_vs_app object
 */
static inline int ip_vs_app_get(struct ip_vs_app *app)
{
	/* test and get the module atomically */
	if (app->module)
		return try_module_get(app->module);
	else
		return 1;
}


static inline void ip_vs_app_put(struct ip_vs_app *app)
{
	if (app->module)
		module_put(app->module);
}


/*
 *	Allocate/initialize app incarnation and register it in proto apps.
 */
int
ip_vs_app_inc_new(struct ip_vs_app *app, __u16 proto, __u16 port)
{
	struct ip_vs_protocol *pp;
	struct ip_vs_app *inc;
	int ret;

	if (!(pp = ip_vs_proto_get(proto)))
		return -EPROTONOSUPPORT;

	if (!pp->unregister_app)
		return -EOPNOTSUPP;

	inc = kmalloc(sizeof(struct ip_vs_app), GFP_KERNEL);
	if (!inc)
		return -ENOMEM;
	memcpy(inc, app, sizeof(*inc));
	INIT_LIST_HEAD(&inc->p_list);
	INIT_LIST_HEAD(&inc->incs_list);
	inc->app = app;
	inc->port = htons(port);
	atomic_set(&inc->usecnt, 0);

	if (app->timeouts) {
		inc->timeout_table =
			ip_vs_create_timeout_table(app->timeouts,
						   app->timeouts_size);
		if (!inc->timeout_table) {
			ret = -ENOMEM;
			goto out;
		}
	}

	ret = pp->register_app(inc);
	if (ret)
		goto out;

	list_add(&inc->a_list, &app->incs_list);
	IP_VS_DBG(9, "%s application %s:%u registered\n",
		  pp->name, inc->name, inc->port);

	return 0;

  out:
	if (inc->timeout_table)
		kfree(inc->timeout_table);
	kfree(inc);
	return ret;
}


/*
 *	Release app incarnation
 */
static void
ip_vs_app_inc_release(struct ip_vs_app *inc)
{
	struct ip_vs_protocol *pp;

	if (!(pp = ip_vs_proto_get(inc->protocol)))
		return;

	if (pp->unregister_app)
		pp->unregister_app(inc);

	IP_VS_DBG(9, "%s App %s:%u unregistered\n",
		  pp->name, inc->name, inc->port);

	list_del(&inc->a_list);

	if (inc->timeout_table != NULL)
		kfree(inc->timeout_table);
	kfree(inc);
}


/*
 *	Get reference to app inc (only called from softirq)
 *
 */
int ip_vs_app_inc_get(struct ip_vs_app *inc)
{
	int result;

	atomic_inc(&inc->usecnt);
	if (unlikely((result = ip_vs_app_get(inc->app)) != 1))
		atomic_dec(&inc->usecnt);
	return result;
}


/*
 *	Put the app inc (only called from timer or net softirq)
 */
void ip_vs_app_inc_put(struct ip_vs_app *inc)
{
	ip_vs_app_put(inc->app);
	atomic_dec(&inc->usecnt);
}


/*
 *	Register an application incarnation in protocol applications
 */
int
register_ip_vs_app_inc(struct ip_vs_app *app, __u16 proto, __u16 port)
{
	int result;

	down(&__ip_vs_app_mutex);

	result = ip_vs_app_inc_new(app, proto, port);

	up(&__ip_vs_app_mutex);

	return result;
}


/*
 *	ip_vs_app registration routine
 */
int register_ip_vs_app(struct ip_vs_app *app)
{
	/* increase the module use count */
	ip_vs_use_count_inc();

	down(&__ip_vs_app_mutex);

	list_add(&app->a_list, &ip_vs_app_list);

	up(&__ip_vs_app_mutex);

	return 0;
}


/*
 *	ip_vs_app unregistration routine
 *	We are sure there are no app incarnations attached to services
 */
void unregister_ip_vs_app(struct ip_vs_app *app)
{
	struct ip_vs_app *inc;
	struct list_head *l = &app->incs_list;

	down(&__ip_vs_app_mutex);

	while (l->next != l) {
		inc = list_entry(l->next, struct ip_vs_app, a_list);
		ip_vs_app_inc_release(inc);
	}

	list_del(&app->a_list);

	up(&__ip_vs_app_mutex);

	/* decrease the module use count */
	ip_vs_use_count_dec();
}


#if 0000
/*
 *	Get reference to app by name (called from user context)
 */
struct ip_vs_app *ip_vs_app_get_by_name(char *appname)
{
	struct list_head *p;
	struct ip_vs_app *app, *a = NULL;

	down(&__ip_vs_app_mutex);

	list_for_each (p, &ip_vs_app_list) {
		app = list_entry(p, struct ip_vs_app, a_list);
		if (strcmp(app->name, appname))
			continue;

		/* softirq may call ip_vs_app_get too, so the caller
		   must disable softirq on the current CPU */
		if (ip_vs_app_get(app))
			a = app;
		break;
	}

	up(&__ip_vs_app_mutex);

	return a;
}
#endif


/*
 *	Bind ip_vs_conn to its ip_vs_app (called by cp constructor)
 */
int ip_vs_bind_app(struct ip_vs_conn *cp, struct ip_vs_protocol *pp)
{
	return pp->app_conn_bind(cp);
}


/*
 *	Unbind cp from application incarnation (called by cp destructor)
 */
void ip_vs_unbind_app(struct ip_vs_conn *cp)
{
	struct ip_vs_app *inc = cp->app;

	if (!inc)
		return;

	if (inc->unbind_conn)
		inc->unbind_conn(inc, cp);
	if (inc->done_conn)
		inc->done_conn(inc, cp);
	ip_vs_app_inc_put(inc);
	cp->app = NULL;
}


/*
 *	Fixes th->seq based on ip_vs_seq info.
 */
static inline void vs_fix_seq(const struct ip_vs_seq *vseq, struct tcphdr *th)
{
	__u32 seq = ntohl(th->seq);

	/*
	 *	Adjust seq with delta-offset for all packets after
	 *	the most recent resized pkt seq and with previous_delta offset
	 *	for all packets	before most recent resized pkt seq.
	 */
	if (vseq->delta || vseq->previous_delta) {
		if(after(seq, vseq->init_seq)) {
			th->seq = htonl(seq + vseq->delta);
			IP_VS_DBG(9, "vs_fix_seq(): added delta (%d) to seq\n",
				  vseq->delta);
		} else {
			th->seq = htonl(seq + vseq->previous_delta);
			IP_VS_DBG(9, "vs_fix_seq(): added previous_delta "
				  "(%d) to seq\n", vseq->previous_delta);
		}
	}
}


/*
 *	Fixes th->ack_seq based on ip_vs_seq info.
 */
static inline void
vs_fix_ack_seq(const struct ip_vs_seq *vseq, struct tcphdr *th)
{
	__u32 ack_seq = ntohl(th->ack_seq);

	/*
	 * Adjust ack_seq with delta-offset for
	 * the packets AFTER most recent resized pkt has caused a shift
	 * for packets before most recent resized pkt, use previous_delta
	 */
	if (vseq->delta || vseq->previous_delta) {
		/* since ack_seq is the number of octet that is expected
		   to receive next, so compare it with init_seq+delta */
		if(after(ack_seq, vseq->init_seq+vseq->delta)) {
			th->ack_seq = htonl(ack_seq - vseq->delta);
			IP_VS_DBG(9, "vs_fix_ack_seq(): subtracted delta "
				  "(%d) from ack_seq\n", vseq->delta);

		} else {
			th->ack_seq = htonl(ack_seq - vseq->previous_delta);
			IP_VS_DBG(9, "vs_fix_ack_seq(): subtracted "
				  "previous_delta (%d) from ack_seq\n",
				  vseq->previous_delta);
		}
	}
}


/*
 *	Updates ip_vs_seq if pkt has been resized
 *	Assumes already checked proto==IPPROTO_TCP and diff!=0.
 */
static inline void vs_seq_update(struct ip_vs_conn *cp, struct ip_vs_seq *vseq,
				 unsigned flag, __u32 seq, int diff)
{
	/* spinlock is to keep updating cp->flags atomic */
	spin_lock(&cp->lock);
	if (!(cp->flags & flag) || after(seq, vseq->init_seq)) {
		vseq->previous_delta = vseq->delta;
		vseq->delta += diff;
		vseq->init_seq = seq;
		cp->flags |= flag;
	}
	spin_unlock(&cp->lock);
}


/*
 *	Output pkt hook. Will call bound ip_vs_app specific function
 *	called by ipvs packet handler, assumes previously checked cp!=NULL
 *	returns (new - old) skb->len diff.
 */
int ip_vs_app_pkt_out(struct ip_vs_conn *cp, struct sk_buff *skb)
{
	struct ip_vs_app *app;
	int diff;
	struct iphdr *iph;
	struct tcphdr *th;
	__u32 seq;

	/*
	 *	check if application module is bound to
	 *	this ip_vs_conn.
	 */
	if ((app = cp->app) == NULL)
		return 0;

	iph = skb->nh.iph;
	th = (struct tcphdr *)&(((char *)iph)[iph->ihl*4]);

	/*
	 *	Remember seq number in case this pkt gets resized
	 */
	seq = ntohl(th->seq);

	/*
	 *	Fix seq stuff if flagged as so.
	 */
	if (cp->protocol == IPPROTO_TCP) {
		if (cp->flags & IP_VS_CONN_F_OUT_SEQ)
			vs_fix_seq(&cp->out_seq, th);
		if (cp->flags & IP_VS_CONN_F_IN_SEQ)
			vs_fix_ack_seq(&cp->in_seq, th);
	}

	/*
	 *	Call private output hook function
	 */
	if (app->pkt_out == NULL)
		return 0;

	diff = app->pkt_out(app, cp, skb);

	/*
	 *	Update ip_vs seq stuff if len has changed.
	 */
	if (diff != 0 && cp->protocol == IPPROTO_TCP)
		vs_seq_update(cp, &cp->out_seq,
			      IP_VS_CONN_F_OUT_SEQ, seq, diff);

	return diff;
}


/*
 *	Input pkt hook. Will call bound ip_vs_app specific function
 *	called by ipvs packet handler, assumes previously checked cp!=NULL.
 *	returns (new - old) skb->len diff.
 */
int ip_vs_app_pkt_in(struct ip_vs_conn *cp, struct sk_buff *skb)
{
	struct ip_vs_app *app;
	int diff;
	struct iphdr *iph;
	struct tcphdr *th;
	__u32 seq;

	/*
	 *	check if application module is bound to
	 *	this ip_vs_conn.
	 */
	if ((app = cp->app) == NULL)
		return 0;

	iph = skb->nh.iph;
	th = (struct tcphdr *)&(((char *)iph)[iph->ihl*4]);

	/*
	 *	Remember seq number in case this pkt gets resized
	 */
	seq = ntohl(th->seq);

	/*
	 *	Fix seq stuff if flagged as so.
	 */
	if (cp->protocol == IPPROTO_TCP) {
		if (cp->flags & IP_VS_CONN_F_IN_SEQ)
			vs_fix_seq(&cp->in_seq, th);
		if (cp->flags & IP_VS_CONN_F_OUT_SEQ)
			vs_fix_ack_seq(&cp->out_seq, th);
	}

	/*
	 *	Call private input hook function
	 */
	if (app->pkt_in == NULL)
		return 0;

	diff = app->pkt_in(app, cp, skb);

	/*
	 *	Update ip_vs seq stuff if len has changed.
	 */
	if (diff != 0 && cp->protocol == IPPROTO_TCP)
		vs_seq_update(cp, &cp->in_seq,
			      IP_VS_CONN_F_IN_SEQ, seq, diff);

	return diff;
}


/*
 *	/proc/net/ip_vs_app entry function
 */
static int
ip_vs_app_getinfo(char *buffer, char **start, off_t offset, int length)
{
	off_t pos=0;
	int len=0;
	char temp[64];
	struct ip_vs_app *app, *inc;
	struct list_head *e, *i;

	pos = 64;
	if (pos > offset) {
		len += sprintf(buffer+len, "%-63s\n",
			       "prot port    usecnt name");
	}

	down(&__ip_vs_app_mutex);
	list_for_each (e, &ip_vs_app_list) {
		app = list_entry(e, struct ip_vs_app, a_list);
		list_for_each (i, &app->incs_list) {
			inc = list_entry(i, struct ip_vs_app, a_list);

			pos += 64;
			if (pos <= offset)
				continue;
			sprintf(temp, "%-3s  %-7u %-6d %-17s",
				ip_vs_proto_name(inc->protocol),
				ntohs(inc->port),
				atomic_read(&inc->usecnt),
				inc->name);
			len += sprintf(buffer+len, "%-63s\n", temp);
			if (pos >= offset+length)
				goto done;
		}
	}
  done:
	up(&__ip_vs_app_mutex);

	*start = buffer+len-(pos-offset);       /* Start of wanted data */
	len = pos-offset;
	if (len > length)
		len = length;
	if (len < 0)
		len = 0;
	return len;
}


/*
 *	Replace a segment of data with a new segment
 */
int ip_vs_skb_replace(struct sk_buff *skb, int pri,
		      char *o_buf, int o_len, char *n_buf, int n_len)
{
	struct iphdr *iph;
	int diff;
	int o_offset;
	int o_left;

	EnterFunction(9);

	diff = n_len - o_len;
	o_offset = o_buf - (char *)skb->data;
	/* The length of left data after o_buf+o_len in the skb data */
	o_left = skb->len - (o_offset + o_len);

	if (diff <= 0) {
		memmove(o_buf + n_len, o_buf + o_len, o_left);
		memcpy(o_buf, n_buf, n_len);
		skb_trim(skb, skb->len + diff);
	} else if (diff <= skb_tailroom(skb)) {
		skb_put(skb, diff);
		memmove(o_buf + n_len, o_buf + o_len, o_left);
		memcpy(o_buf, n_buf, n_len);
	} else {
		if (pskb_expand_head(skb, skb_headroom(skb), diff, pri))
			return -ENOMEM;
		skb_put(skb, diff);
		memmove(skb->data + o_offset + n_len,
			skb->data + o_offset + o_len, o_left);
		memcpy(skb->data + o_offset, n_buf, n_len);
	}

	/* must update the iph total length here */
	iph = skb->nh.iph;
	iph->tot_len = htons(skb->len);

	LeaveFunction(9);
	return 0;
}


int ip_vs_app_init(void)
{
	/* we will replace it with proc_net_ipvs_create() soon */
	proc_net_create("ip_vs_app", 0, ip_vs_app_getinfo);
	return 0;
}


void ip_vs_app_cleanup(void)
{
	proc_net_remove("ip_vs_app");
}