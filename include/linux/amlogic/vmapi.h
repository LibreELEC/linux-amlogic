#ifndef VM_API_INCLUDE_
#define VM_API_INCLUDE_

typedef struct vm_output_para{
    int width;
    int height;
    int bytesperline;
    int v4l2_format;
    int index;
    int v4l2_memory;
    int zoom;     // set -1 as invalid
    int mirror;   // set -1 as invalid
    int angle;
    unsigned vaddr;
    unsigned int ext_canvas;
}vm_output_para_t;
struct videobuf_buffer;
typedef struct vm_init_s {
    size_t vm_buf_size;
    struct page *vm_pages;
    resource_size_t buffer_start;
    struct io_mapping *mapping;
    bool isused;
    bool mem_alloc_succeed;
}vm_init_t;
int vm_fill_this_buffer(struct videobuf_buffer* vb , vm_output_para_t* para, vm_init_t* info);
int vm_fill_buffer(struct videobuf_buffer* vb , vm_output_para_t* para);

#ifdef CONFIG_CMA

int vm_init_buf(size_t size);
int vm_init_resource(size_t size, vm_init_t* info);
void vm_deinit_buf(void);
void vm_deinit_resource(vm_init_t* info);
void vm_reserve_cma(void);
#endif

#endif /* VM_API_INCLUDE_ */
