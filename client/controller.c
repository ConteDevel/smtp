#include "controller.h"
#include "config.client.h"

void controller_init(controller_t * p, int worker_cnt)
{
    ASSERT(p);
    ASSERT(worker_cnt > 0);

    p->worker_ptr = malloc(sizeof(*p->worker_ptr) * worker_cnt);
    ASSERT(p->worker_ptr && "Error alloc memory");

    p->worker_cnt = worker_cnt;
    for (int i = 0; i < p->worker_cnt; ++i)
    {
        worker_init(&p->worker_ptr[i].worker);
        p->worker_ptr[i].assigned_domains = map_create();
        p->worker_ptr[i].assigned_domains->key_deleter = sl_clear;
    }

    p->is_worked = true;
}

void controller_clear(controller_t * p)
{
    ASSERT(p);
    ASSERT(p->worker_ptr);

    for (int i = 0; i < p->worker_cnt; ++i)
    {
        worker_clear(&p->worker_ptr[i].worker);
        map_free(p->worker_ptr[i].assigned_domains);
    }

    free(p->worker_ptr);
}

typedef struct file_rec
{
    sl_t              name;
    struct file_rec * next;
} file_t;

sl_t get_user_dir(const sl_t in, const sl_t maildir)
{
    return sl_create(FMT_SL "/" FMT_SL "/Maildir/new/", ARG_SL(maildir), ARG_SL(in));
}

file_t * get_dir_queue(const sl_t maildir)
{
    DIR * directory = opendir(maildir.s);
    if (!directory)
    {
        //LOG_E("Users not found");
        return NULL;
    }

    struct dirent *dir;

    file_t * dir_queue = NULL;
    file_t * ptr       = dir_queue;
    while ((dir = readdir(directory)) != NULL)
    {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        if (dir_queue == NULL)
        {
            dir_queue = malloc(sizeof(*dir_queue));
            ASSERT(dir_queue && "Error memory alloc");

            dir_queue->next = NULL;
            dir_queue->name = get_user_dir(SL(dir->d_name, strlen(dir->d_name)), maildir);

            // LOG_I(FMT_SL, ARG_SL(queue->name));

            // sl_free(&queue->name);
            // free(queue);

            ptr = dir_queue;
        }
        else
        {
            ASSERT(ptr);
            ASSERT(ptr->next == NULL);

            file_t * next_file = malloc(sizeof(*next_file));
            ASSERT(next_file && "Error memory alloc");

            next_file->next = NULL;
            next_file->name = get_user_dir(SL(dir->d_name, strlen(dir->d_name)), maildir);

            ptr->next = next_file;
            ptr       = next_file;
            // LOG_I(FMT_SL, ARG_SL(queue->name));
        }
    }

    closedir(directory);

    return dir_queue;
}

void create_tmp_dirs(file_t * dir_queue)
{
    file_t * dir_ptr = dir_queue;
    while (dir_ptr != NULL)
    {
        sl_t tmp_dir_name = sl_create("%.*s/tmp", dir_ptr->name.l - 6, dir_ptr->name.s);
        DIR * directory = opendir(tmp_dir_name.s);

        LOG_I("Created directory " FMT_SL, ARG_SL(tmp_dir_name));
        if (!directory)
        {
            ASSERT(errno == ENOENT);
            mkdir(tmp_dir_name.s, 0777);
        }

        closedir(directory);
        sl_free(&tmp_dir_name);

        dir_ptr = dir_ptr->next;        
    }
}

file_t * get_letter_files(file_t * dir_queue)
{
    file_t * file_queue = NULL;
    file_t * file_ptr   = file_queue;

    file_t * dir_ptr = dir_queue;
    while (dir_ptr != NULL)
    {
        DIR * directory = opendir(dir_ptr->name.s);
        if (directory)
        {
            struct dirent *dir;
            while ((dir = readdir(directory)) != NULL)
            {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                    continue;

                if (file_queue == NULL)
                {
                    file_queue = malloc(sizeof(*file_queue));
                    ASSERT(file_queue && "Error memory alloc");

                    file_queue->next = NULL;
                    file_queue->name = sl_create(FMT_SL FMT_SL, ARG_SL(dir_queue->name),
                                                                ARG_SL(SL(dir->d_name, strlen(dir->d_name))));

                    file_ptr = file_queue;
                }
                else
                {
                    ASSERT(file_ptr);
                    ASSERT(file_ptr->next == NULL);

                    file_t * next_file = malloc(sizeof(*next_file));
                    ASSERT(next_file && "Error memory alloc");

                    next_file->next = NULL;
                    next_file->name = sl_create(FMT_SL FMT_SL, ARG_SL(dir_queue->name),
                                                               ARG_SL(SL(dir->d_name, strlen(dir->d_name))));

                    file_ptr->next = next_file;
                    file_ptr       = next_file;
                }
            }
        }
        else
            LOG_E("Error open directory " FMT_SL, ARG_SL(dir_ptr->name));

        closedir(directory);

        dir_ptr = dir_ptr->next;
    }

    // free dir queue
    dir_ptr = dir_queue;
    while (dir_ptr != NULL)
    {
        sl_free(&dir_ptr->name);
        dir_queue = dir_ptr;

        dir_ptr = dir_ptr->next;
        free(dir_queue);
    }

    return file_queue;
}

void read_letter_info_from_file(FILE * file, sl_t * domain, sl_t * from, sl_t * to)
{
    const uint32_t FRAME_SIZE = 256;

    char buffer[FRAME_SIZE];
    sl_t TO   = SLLIT("To:");
    sl_t FROM = SLLIT("From:");
    TO.l   -= 1;
    FROM.l -= 1;

    while (true)
    {
        if (fgets(buffer, FRAME_SIZE, file) == NULL)
            break;

        sl_t str = SL(buffer, strlen(buffer));
        if (str.s[str.l - 2] != '\r' && str.s[str.l - 1] != '\n')
            continue;

        if (strncmp(str.s, TO.s, MIN(str.l, TO.l)) == 0)
        {
            char * pos = strstr(str.s, "@");
            if (pos)
            {
                sl_free(domain);
                *domain = sl_dup(SL(pos + 1, strlen(pos) - 2));
            }

            pos = str.s + TO.l + 1;
            if (str.l > TO.l)
            {
                sl_free(to);
                *to = sl_dup(SL(pos, strlen(pos) - 1));
            }
        }
        else if (strncmp(str.s, FROM.s, MIN(str.l, FROM.l)) == 0)
        {
            char * pos = str.s + FROM.l + 1;
            if (str.l > FROM.l)
            {
                sl_free(from);
                *from = sl_dup(SL(pos, strlen(pos) - 1));
            }
        } 
        else
            continue;
    }
}

sl_t change_path(const sl_t path, char * from, char * _to)
{
    sl_t to      = SL(_to,  strlen(_to));
    sl_t pattern = SL(from, strlen(from));
    pattern.l -= 1;

    char          *pos = strstr(path.s, pattern.s);
    const sl_t  before = SL(path.s, pos - path.s);
    const sl_t   after = SL(pos + pattern.l + 1, path.l - before.l - pattern.l - 1);
    
    sl_t new_path = sl_create(FMT_SL FMT_SL FMT_SL, ARG_SL(before), ARG_SL(to), ARG_SL(after));

    return new_path;
}

void assigne_task_to_worker(worker_wrapper_t * wrapper, task_t * task)
{
    ASSERT(wrapper);
    ASSERT(task);
    
    sl_t new_path = change_path(task->path, "/new/", "/tmp/");
    
    int rc;
    rc = rename(task->path.s, new_path.s);
    // rc = 0;
    if (rc < 0)
    {
        LOG_E("Fail to move file from " FMT_SL " to " FMT_SL,
              ARG_SL(task->path), ARG_SL(new_path));
        sl_free(&new_path);
        task_free(task);
        free(task);
        return;
    }

    sl_free(&task->path);
    task->path = new_path;

    sem_wait(&wrapper->worker.new_tasks_lock);
    task->next_task = wrapper->worker.new_tasks;
    wrapper->worker.new_tasks = task;
    sem_post(&wrapper->worker.new_tasks_lock);

    long leave_cnt = (long)map_get(wrapper->assigned_domains, task->domain);
    leave_cnt += 1;
    map_put(wrapper->assigned_domains, sl_dup(task->domain), (void *)leave_cnt);

    LOG_I("Assigne task " FMT_SL " to worker %lu", ARG_SL(task->path), wrapper->worker.thread);
}

void assigne_task(controller_t * controller, task_t * task)
{
    ASSERT(controller);
    ASSERT(task);

    //LOG_D("dom = " FMT_SL, ARG_SL(task->domain));

    bool found = false;
    long min_task_cnt = INT32_MAX;
    int min_index     = INT32_MAX;
    for (int i = 0; i < controller->worker_cnt; ++i)
    {
        worker_wrapper_t * wrapper = controller->worker_ptr + i;
        if (map_get(wrapper->assigned_domains, task->domain))
        {
            assigne_task_to_worker(wrapper, task);
            found = true;
            break;
        }

        long cnt = 0;
        MAP_FOREACH(wrapper->assigned_domains, entry)
        {
            LOG_D(FMT_SL, ARG_SL(entry->key));
            cnt += (long) entry->value;
        }

        if (cnt < min_task_cnt)
        {
            min_task_cnt = cnt;
            min_index    = i;
        }
    }

    if (!found)
    {
        ASSERT(min_index < controller->worker_cnt);
        assigne_task_to_worker(controller->worker_ptr + min_index, task);
    }
}

void distribute_tasks(controller_t * controller, file_t * letter_files)
{
    file_t * ptr = letter_files;
    while (ptr != NULL)
    {
        //LOG_I(FMT_SL, ARG_SL(ptr->name));

        sl_t domain = SLNULL;
        sl_t from   = SLNULL;
        sl_t to     = SLNULL;
        FILE * file = fopen(ptr->name.s, "r");
        if (file)
            read_letter_info_from_file(file, &domain, &from, &to);
        else
            LOG_E("Error read file " FMT_SL, ARG_SL(ptr->name));

        fclose(file);

        if (domain.s != NULL && from.s != NULL && to.s != NULL)
        {
            task_t * task = malloc(sizeof(*task));
            ASSERT(task && "Error memory alloc");
            
            task->from       = from;
            task->to         = to;
            task->domain     = domain;
            task->path       = sl_dup(ptr->name);
            task->next_task  = NULL;
            task->error_desc = SLNULL;

            assigne_task(controller, task);
        }
        else
        {
            if (domain.s == NULL)
                LOG_E("Not found target domain in " FMT_SL, ARG_SL(ptr->name));
            if (from.s == NULL)
                LOG_E("Not found target 'from' in " FMT_SL, ARG_SL(ptr->name));
            if (to.s == NULL)
                LOG_E("Not found target 'to' in " FMT_SL, ARG_SL(ptr->name));
        }

        sl_free(&ptr->name);
        letter_files = ptr;

        ptr = ptr->next;
        free(letter_files);
    }
}

void update_task_condition(controller_t * p)
{
    for (int i = 0; i < p->worker_cnt; ++i)
    {
        worker_wrapper_t * wrapper = p->worker_ptr + i;
        worker_t * worker = &wrapper->worker;

        task_t * finished_tasks = NULL;

        sem_wait(&worker->finished_tasks_lock);
        finished_tasks = worker->finished_tasks;
        worker->finished_tasks = NULL;
        sem_post(&worker->finished_tasks_lock);

        while (finished_tasks != NULL)
        {
            long leave_cnt = (long)map_get(wrapper->assigned_domains, 
                                           finished_tasks->domain);

            ASSERT(leave_cnt > 0);
            leave_cnt -= 1;
            if (leave_cnt == 0)
            {
                map_remove(wrapper->assigned_domains,
                           finished_tasks->domain);
            }
            else
            {
                map_put(wrapper->assigned_domains, finished_tasks->domain,
                        (void *)(long)leave_cnt);
            }

            task_t   * old = finished_tasks;
            finished_tasks = finished_tasks->next_task;

            if (old->error_desc.s)
            {
                sl_t new_path = change_path(old->path, "/tmp/", "/new/");
                
                int rc;
                rc = rename(old->path.s, new_path.s);
                if (rc < 0)
                {
                    LOG_F("Fail to move file from " FMT_SL " to " FMT_SL,
                          ARG_SL(old->path), ARG_SL(new_path));
                    ASSERT(false);
                }

                LOG_I("Task was failed because '" FMT_SL "', move related letter " FMT_SL " to " FMT_SL, 
                       ARG_SL(old->error_desc), ARG_SL(old->path), ARG_SL(new_path));
                sl_free(&old->path);
                old->path = new_path;
            }
            else
            {
                sl_t new_path = change_path(old->path, "/tmp/", "/sended/");
                
                int rc;
                rc = rename(old->path.s, new_path.s);
                if (rc < 0)
                {
                    LOG_F("Fail to move file from " FMT_SL " to " FMT_SL,
                          ARG_SL(old->path), ARG_SL(new_path));
                    ASSERT(false);
                }

                LOG_I("Task was failed because '" FMT_SL "', move related letter " FMT_SL " to " FMT_SL, 
                       ARG_SL(old->error_desc), ARG_SL(old->path), ARG_SL(new_path));
                sl_free(&old->path);
                old->path = new_path;
            }

            task_free(old);
        }
    }
}

void controller_run(controller_t * p, sl_t maildir)
{
    ASSERT(p);

    while (p->is_worked)
    {
        LOG_I("Loop iteration start");

        update_task_condition(p);

        file_t * new_dirs = get_dir_queue(maildir);
        create_tmp_dirs(new_dirs);

        file_t * letter_files = get_letter_files(new_dirs);
        distribute_tasks(p, letter_files);

        LOG_I("Loop iteration finished");
        sleep(CONF_CL_UPDATE_MAILDIR_TIME_SEC);
    }

    for (int i = 0; i < p->worker_cnt; ++i)
        p->worker_ptr[i].worker.is_worked = false;

    for (int i = 0; i < p->worker_cnt; ++i)
        pthread_join(p->worker_ptr[i].worker.thread, NULL);
}