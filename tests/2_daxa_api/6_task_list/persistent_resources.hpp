#pragma once

#include "common.hpp"

namespace tests
{
    void sharing_persistent_buffer()
    {
        // TEST:
        //  1) Create single persistent buffer
        //  2) Record two task lists (A, B)
        //  3) Task list A:
        //      writes persistent buffer
        //  4) Task list B:
        //      reads persistent buffer
        //  6) Call execute task lists in this order - A -> A -> B -> B
        //  5) Check persistent synch of task lists A and B 
        //      1) INIT -> A - Expected - First execution of A syncs on nothing
        //      2) A    -> A - Expected - Second execution of A syncs on SHADER_WRITE -> SHADER_WRITE
        //      3) A    -> B - Expected - First execution of B syncs on SHADER_WRITE -> SHADER_READ
        //      4) B    -> B - Expected - Second execution of B has no sync
        daxa::Context daxa_ctx = daxa::create_context({ .enable_validation = false, });
        daxa::Device device = daxa_ctx.create_device({ .name = "device", });
        auto buffer = device.create_buffer({
           .size = 1,
           .allocate_info = daxa::MemoryFlagBits::DEDICATED_MEMORY,
           .name = "actual buffer",
        });

        auto persistent_task_buffer = daxa::TaskBuffer(daxa::TaskBufferInfo{
           .initial_buffers = {.buffers = {&buffer, 1}},
           .name = "persistent buffer",
        });
 
        auto task_list_A = daxa::TaskList({
           .device = device,
           .record_debug_information = true,
           .name = "task_list_a",
        });

        auto task_list_B = daxa::TaskList({
           .device = device,
           .record_debug_information = true,
           .name = "task_list_b",
        });

        task_list_A.use_persistent_buffer(persistent_task_buffer);
        task_list_B.use_persistent_buffer(persistent_task_buffer);
        task_list_A.add_task({
           .uses = {daxa::TaskBufferUse<daxa::TaskBufferAccess::SHADER_WRITE>{persistent_task_buffer}},
           .task = [&](daxa::TaskInterface const &) { },
           .name = "write persistent buffer",
        });
        task_list_A.submit({});
        task_list_A.complete({});

        task_list_B.add_task({
           .uses = {daxa::TaskBufferUse<daxa::TaskBufferAccess::SHADER_READ>{persistent_task_buffer}},
           .task = [&](daxa::TaskInterface const &) { },
           .name = "read persistent buffer",
        });
        task_list_B.submit({});
        task_list_B.complete({});

        task_list_A.execute({});
        std::cout << task_list_A.get_debug_string() << std::endl;
        task_list_A.execute({});
        std::cout << task_list_A.get_debug_string() << std::endl;
        task_list_B.execute({});
        std::cout << task_list_B.get_debug_string() << std::endl;
        task_list_B.execute({});
        std::cout << task_list_B.get_debug_string() << std::endl;

        device.wait_idle();
        device.destroy_buffer(buffer);
        device.collect_garbage();
    }
    void sharing_persistent_image()
    {
        // TEST:
        //  1) Create single persistent image
        //  2) Record two task lists (A, B)
        //  3) Task list A:
        //      writes persistent image
        //      uses persistent image as color attachment
        //  4) Task list B:
        //      reads persistent image
        //  6) Call execute task lists in this order - A -> A -> B -> B
        //  5) Check persistent synch of task lists A and B 
        //      1) INIT -> A - Expected - First execution of A transitions image from UNDEFINED to WRITE
        //      2) A    -> A - Expected - Second execution of  A transitions image from COLOR_ATTACHMENT to WRITE
        //      3) A    -> B - Expected - First execution of B transitions image from WRITE to READ
        //      4) B    -> B - Expected - Second execution of B has no transitions
        daxa::Context daxa_ctx = daxa::create_context({ .enable_validation = false, });
        daxa::Device device = daxa_ctx.create_device({ .name = "device", });
        // We need an actual image, as task list will try to populate its image view cache.
        // It will error out when it detects that there are no runtime images for a task image when updating the view cache.
        auto image = device.create_image({
           .size = {1, 1, 1},
           .array_layer_count = 1,
           .usage = daxa::ImageUsageFlagBits::SHADER_READ_WRITE,
           .name = "actual image",
        });

        auto persistent_task_image = daxa::TaskImage(
            daxa::TaskImageInfo{
                .initial_images = {.images = {&image, 1}},
                .swapchain_image = false,
                .name = "image",
        });
 
        auto task_list_A = daxa::TaskList({
           .device = device,
           .record_debug_information = true,
           .name = "task_list_a",
        });

        task_list_A.use_persistent_image(persistent_task_image);
        task_list_A.add_task({
           .uses = {daxa::TaskImageUse<daxa::TaskImageAccess::SHADER_WRITE>{persistent_task_image}},
           .task = [&](daxa::TaskInterface const &) { },
           .name = "write persistent image",
        });
        task_list_A.add_task({
           .uses = {daxa::TaskImageUse<daxa::TaskImageAccess::COLOR_ATTACHMENT>{persistent_task_image}},
           .task = [&](daxa::TaskInterface const &) { },
           .name = "persistent image - color attachment",
        });
        task_list_A.submit({});
        task_list_A.complete({});

        auto task_list_B = daxa::TaskList({
           .device = device,
           .record_debug_information = true,
           .name = "task_list_b",
        });
        task_list_B.use_persistent_image(persistent_task_image);
        task_list_B.add_task({
           .uses = {daxa::TaskImageUse<daxa::TaskImageAccess::SHADER_READ>{persistent_task_image}},
           .task = [&](daxa::TaskInterface const &) { },
           .name = "read persistent image",
        });
        task_list_B.submit({});
        task_list_B.complete({});

        task_list_A.execute({});
        std::cout << task_list_A.get_debug_string() << std::endl;
        task_list_A.execute({});
        std::cout << task_list_A.get_debug_string() << std::endl;
        task_list_B.execute({});
        std::cout << task_list_B.get_debug_string() << std::endl;
        task_list_B.execute({});
        std::cout << task_list_B.get_debug_string() << std::endl;

        device.wait_idle();
        device.destroy_image(image);
        device.collect_garbage();
    }
}

